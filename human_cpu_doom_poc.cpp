#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

// Human CPU Doom-like PoC
// -----------------------
// This program intentionally does NOT have to compute the whole next frame.
// It acts as external memory + display. The human is the CPU:
//   1) chooses action
//   2) manually computes next player state
//   3) manually computes ray distances for 16 screen columns
//   4) program stores those values and renders an ASCII frame
//
// Optional commands provide hints/auto-fill/validation, but the normal mode is manual.
// Build: g++ -std=c++17 -O2 human_cpu_doom_poc.cpp -o human_doom

struct Player {
    int x = 2;      // grid cell x
    int y = 2;      // grid cell y
    int dir = 0;    // 0=N, 1=E, 2=S, 3=W
};

struct Memory {
    Player p;
    Player next;
    char action = '.';
    std::array<int, 16> dist{}; // ray hit distance in grid cells, 1..9 recommended
};

static const std::vector<std::string> MAP = {
    "############",
    "#..........#",
    "#.####.....#",
    "#.#..#..#..#",
    "#.#..#..#..#",
    "#....#.....#",
    "#..###..####",
    "#..........#",
    "############"
};

static const char* DIR_NAME[4] = {"N", "E", "S", "W"};
static const int DX[4] = {0, 1, 0, -1};
static const int DY[4] = {-1, 0, 1, 0};

static bool inside(int x, int y) {
    return y >= 0 && y < (int)MAP.size() && x >= 0 && x < (int)MAP[y].size();
}

static bool wall(int x, int y) {
    return !inside(x, y) || MAP[y][x] == '#';
}

static std::string trim(std::string s) {
    while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
    return s;
}

static void printMap(const Player& p) {
    std::cout << "\nMAP / EXTERNAL MEMORY\n";
    std::cout << "Legend: # wall, . empty, ^>v< player\n\n";
    for (int y = 0; y < (int)MAP.size(); ++y) {
        for (int x = 0; x < (int)MAP[y].size(); ++x) {
            if (x == p.x && y == p.y) {
                static const char icon[4] = {'^', '>', 'v', '<'};
                std::cout << icon[p.dir];
            } else {
                std::cout << MAP[y][x];
            }
        }
        std::cout << "\n";
    }
}

static void printRegisters(const Memory& m) {
    std::cout << "\nREGISTERS\n";
    std::cout << " P.X=" << m.p.x << " P.Y=" << m.p.y << " P.DIR=" << DIR_NAME[m.p.dir] << "(" << m.p.dir << ")\n";
    std::cout << " ACTION=" << m.action << "\n";
    std::cout << " NEXT.X=" << m.next.x << " NEXT.Y=" << m.next.y << " NEXT.DIR=" << DIR_NAME[m.next.dir] << "(" << m.next.dir << ")\n";
    std::cout << " DIST[16]=";
    for (int d : m.dist) std::cout << ' ' << d;
    std::cout << "\n";
}

static Player autoNext(Player p, char action) {
    action = (char)std::tolower((unsigned char)action);
    Player n = p;
    if (action == 'a') n.dir = (p.dir + 3) % 4;
    else if (action == 'd') n.dir = (p.dir + 1) % 4;
    else if (action == 'w') {
        int nx = p.x + DX[p.dir];
        int ny = p.y + DY[p.dir];
        if (!wall(nx, ny)) { n.x = nx; n.y = ny; }
    } else if (action == 's') {
        int nx = p.x - DX[p.dir];
        int ny = p.y - DY[p.dir];
        if (!wall(nx, ny)) { n.x = nx; n.y = ny; }
    }
    return n;
}

// Simple ray basis: 16 columns approximated by only three ray directions: left-ish, center, right-ish.
// To keep it human-computable, each column asks for distance to wall along one of these grid rays.
// This is not real Doom rendering; it is a deliberately tiny raycaster-like proof of concept.
struct RayDir { int dx; int dy; const char* name; };

static RayDir rotateRay(RayDir r, int dir) {
    // rotate vector from N-facing local coords to world coords.
    // Local N frame: center=(0,-1), left=(-1,-1), right=(1,-1).
    int x = r.dx, y = r.dy;
    for (int i = 0; i < dir; ++i) { int nx = -y; int ny = x; x = nx; y = ny; }
    return {x, y, r.name};
}

static RayDir localRayForColumn(int col) {
    if (col < 5) return {-1, -1, "LEFT_DIAGONAL"};
    if (col < 11) return {0, -1, "CENTER"};
    return {1, -1, "RIGHT_DIAGONAL"};
}

static int autoDistance(Player p, int col) {
    RayDir r = rotateRay(localRayForColumn(col), p.dir);
    int x = p.x;
    int y = p.y;
    for (int d = 1; d <= 9; ++d) {
        x += r.dx;
        y += r.dy;
        if (wall(x, y)) return d;
    }
    return 9;
}

static void printRayTask(const Memory& m) {
    std::cout << "\nRAY TASK\n";
    std::cout << "Fill DIST[0..15]. Distance = number of grid steps from NEXT position to first wall.\n";
    std::cout << "Columns use only 3 ray directions to keep it human-computable:\n";
    for (int c = 0; c < 16; ++c) {
        RayDir local = localRayForColumn(c);
        RayDir world = rotateRay(local, m.next.dir);
        std::cout << " col " << std::setw(2) << c
                  << ": " << std::setw(14) << local.name
                  << " world_step=(" << std::setw(2) << world.dx << "," << std::setw(2) << world.dy << ")\n";
    }
}

static void renderFrame(const Memory& m) {
    const int W = 16;
    const int H = 12;
    std::array<std::string, H> screen;
    for (auto& row : screen) row = std::string(W, ' ');

    for (int x = 0; x < W; ++x) {
        int d = std::clamp(m.dist[x], 1, 9);
        int wallH = std::clamp(12 / d, 1, H);
        int top = (H - wallH) / 2;
        int bot = top + wallH;
        char shade = d <= 1 ? '@' : d <= 2 ? '#' : d <= 3 ? 'X' : d <= 5 ? '+' : '.';
        for (int y = 0; y < H; ++y) {
            if (y < top) screen[y][x] = ' ';       // ceiling
            else if (y < bot) screen[y][x] = shade; // wall slice
            else screen[y][x] = '_';              // floor
        }
    }

    std::cout << "\nFRAMEBUFFER / ASCII DISPLAY\n";
    std::cout << '+' << std::string(W, '-') << "+\n";
    for (const auto& row : screen) std::cout << '|' << row << "|\n";
    std::cout << '+' << std::string(W, '-') << "+\n";
}

static bool parseThreeInts(const std::string& s, int& a, int& b, int& c) {
    std::istringstream iss(s);
    return bool(iss >> a >> b >> c);
}

static bool parseSixteenInts(const std::string& s, std::array<int, 16>& out) {
    std::istringstream iss(s);
    for (int i = 0; i < 16; ++i) {
        if (!(iss >> out[i])) return false;
    }
    return true;
}

static void validateNext(const Memory& m) {
    Player correct = autoNext(m.p, m.action);
    bool ok = (correct.x == m.next.x && correct.y == m.next.y && correct.dir == m.next.dir);
    if (ok) {
        std::cout << "[OK] NEXT registers match hidden reference.\n";
    } else {
        std::cout << "[MISMATCH] Expected NEXT.X=" << correct.x
                  << " NEXT.Y=" << correct.y
                  << " NEXT.DIR=" << DIR_NAME[correct.dir] << "(" << correct.dir << ")\n";
    }
}

static void validateDistances(const Memory& m) {
    int errors = 0;
    for (int c = 0; c < 16; ++c) {
        int expected = autoDistance(m.next, c);
        if (m.dist[c] != expected) {
            if (errors == 0) std::cout << "[DIST MISMATCHES]\n";
            std::cout << " col " << c << ": got " << m.dist[c] << ", expected " << expected << "\n";
            ++errors;
        }
    }
    if (errors == 0) std::cout << "[OK] DIST[16] matches hidden reference.\n";
}

static void help() {
    std::cout << "\nCOMMANDS\n"
              << " map           show external memory map\n"
              << " regs          show registers\n"
              << " action X      set action: w forward, s back, a turn left, d turn right, . none\n"
              << " next x y dir  manually write NEXT registers; dir: 0=N 1=E 2=S 3=W\n"
              << " rays          show ray directions for each column\n"
              << " dist 16 nums  manually write DIST[0..15], e.g. dist 3 3 3 2 ...\n"
              << " frame         render ASCII frame from memory\n"
              << " commit        P := NEXT, then show frame\n"
              << " validate      compare your manual registers/distances with hidden reference\n"
              << " hintnext      print correct NEXT only, for learning/debug\n"
              << " hintdist      print correct DIST[16] only, for learning/debug\n"
              << " auto          auto-fill NEXT and DIST, then render/commit; not human-CPU mode\n"
              << " help          show this help\n"
              << " quit          exit\n";
}

int main() {
    Memory mem;
    mem.next = mem.p;
    mem.dist.fill(4);

    std::cout << "Human CPU Doom-like PoC\n";
    std::cout << "The program is external memory/display. You are the CPU.\n";
    help();
    printMap(mem.p);
    printRegisters(mem);
    renderFrame(mem);

    std::string line;
    while (true) {
        std::cout << "\nhuman-cpu> ";
        if (!std::getline(std::cin, line)) break;
        line = trim(line);
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        for (char& ch : cmd) ch = (char)std::tolower((unsigned char)ch);

        if (cmd == "quit" || cmd == "exit" || cmd == "q") break;
        else if (cmd == "help") help();
        else if (cmd == "map") printMap(mem.p);
        else if (cmd == "regs") printRegisters(mem);
        else if (cmd == "rays") printRayTask(mem);
        else if (cmd == "frame") renderFrame(mem);
        else if (cmd == "action") {
            char a = '.';
            if (!(iss >> a)) {
                std::cout << "Usage: action w|s|a|d|.\n";
                continue;
            }
            a = (char)std::tolower((unsigned char)a);
            if (std::string("wsad.").find(a) == std::string::npos) {
                std::cout << "Invalid action. Use w/s/a/d/.\n";
                continue;
            }
            mem.action = a;
            std::cout << "ACTION := " << mem.action << "\n";
            std::cout << "Now compute and enter: next x y dir\n";
        }
        else if (cmd == "next") {
            int x, y, d;
            if (!(iss >> x >> y >> d) || d < 0 || d > 3) {
                std::cout << "Usage: next x y dir   where dir=0..3\n";
                continue;
            }
            mem.next = {x, y, d};
            std::cout << "NEXT written. Now compute and enter DIST[16], or type rays.\n";
        }
        else if (cmd == "dist") {
            std::string rest;
            std::getline(iss, rest);
            std::array<int,16> tmp{};
            if (!parseSixteenInts(rest, tmp)) {
                std::cout << "Usage: dist d0 d1 ... d15\n";
                continue;
            }
            mem.dist = tmp;
            std::cout << "DIST[16] written. Type frame or commit.\n";
        }
        else if (cmd == "commit") {
            mem.p = mem.next;
            printMap(mem.p);
            renderFrame(mem);
            std::cout << "Committed. Choose next action.\n";
        }
        else if (cmd == "validate") {
            validateNext(mem);
            validateDistances(mem);
        }
        else if (cmd == "hintnext") {
            Player n = autoNext(mem.p, mem.action);
            std::cout << "NEXT reference: " << n.x << ' ' << n.y << ' ' << n.dir
                      << "  // " << DIR_NAME[n.dir] << "\n";
        }
        else if (cmd == "hintdist") {
            std::cout << "DIST reference:";
            for (int c = 0; c < 16; ++c) std::cout << ' ' << autoDistance(mem.next, c);
            std::cout << "\n";
        }
        else if (cmd == "auto") {
            mem.next = autoNext(mem.p, mem.action);
            for (int c = 0; c < 16; ++c) mem.dist[c] = autoDistance(mem.next, c);
            mem.p = mem.next;
            printMap(mem.p);
            printRegisters(mem);
            renderFrame(mem);
        }
        else {
            std::cout << "Unknown command. Type help.\n";
        }
    }

    std::cout << "Bye.\n";
    return 0;
}
