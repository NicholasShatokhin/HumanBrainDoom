#include <QtWidgets>

#include <array>
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>

struct Player {
    int x = 2;
    int y = 2;
    int dir = 0; // 0=N 1=E 2=S 3=W
};

struct Memory {
    Player p;
    Player next;
    QChar action = '.';
    std::array<int, 16> dist{};
};

static const std::vector<QString> kMap = {
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

static const char* kDirName[4] = {"N", "E", "S", "W"};
static const int kDx[4] = {0, 1, 0, -1};
static const int kDy[4] = {-1, 0, 1, 0};

static bool inside(int x, int y) {
    return y >= 0 && y < (int)kMap.size() && x >= 0 && x < kMap[(size_t)y].size();
}

static bool wall(int x, int y) {
    return !inside(x, y) || kMap[(size_t)y][x] == '#';
}

static Player autoNext(Player p, QChar action) {
    Player n = p;
    const QChar a = action.toLower();
    if (a == 'a') n.dir = (p.dir + 3) % 4;
    else if (a == 'd') n.dir = (p.dir + 1) % 4;
    else if (a == 'w') {
        int nx = p.x + kDx[p.dir];
        int ny = p.y + kDy[p.dir];
        if (!wall(nx, ny)) { n.x = nx; n.y = ny; }
    } else if (a == 's') {
        int nx = p.x - kDx[p.dir];
        int ny = p.y - kDy[p.dir];
        if (!wall(nx, ny)) { n.x = nx; n.y = ny; }
    }
    return n;
}

struct RayDir {
    int dx = 0;
    int dy = -1;
    QString name = "CENTER";
};

static RayDir localRayForColumn(int col) {
    if (col < 5) return {-1, -1, "LEFT_DIAGONAL"};
    if (col < 11) return {0, -1, "CENTER"};
    return {1, -1, "RIGHT_DIAGONAL"};
}

static RayDir rotateRay(RayDir r, int dir) {
    int x = r.dx;
    int y = r.dy;
    for (int i = 0; i < dir; ++i) {
        int nx = -y;
        int ny = x;
        x = nx;
        y = ny;
    }
    r.dx = x;
    r.dy = y;
    return r;
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

class FrameBufferWidget final : public QWidget {
public:
    explicit FrameBufferWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumSize(420, 280);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void setDistances(const std::array<int, 16>& dist) {
        m_dist = dist;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), QColor(15, 15, 15));
        p.setRenderHint(QPainter::Antialiasing, false);

        const int cols = 16;
        const int margin = 12;
        const int W = width() - margin * 2;
        const int H = height() - margin * 2;
        const int colW = std::max(1, W / cols);
        const int cx0 = margin;
        const int cy0 = margin;

        // ceiling/floor
        p.fillRect(QRect(cx0, cy0, colW * cols, H / 2), QColor(35, 35, 45));
        p.fillRect(QRect(cx0, cy0 + H / 2, colW * cols, H / 2), QColor(30, 30, 30));

        for (int c = 0; c < cols; ++c) {
            int d = std::clamp(m_dist[(size_t)c], 1, 9);
            double hFrac = std::clamp(1.20 / double(d), 0.08, 1.0);
            int wallH = std::max(4, int(H * hFrac));
            int top = cy0 + (H - wallH) / 2;
            int left = cx0 + c * colW;
            int shade = 230 - d * 18;
            shade = std::clamp(shade, 60, 220);
            QColor color(shade, shade, shade);
            p.fillRect(QRect(left + 1, top, colW - 2, wallH), color);
            p.setPen(QColor(0, 0, 0, 70));
            p.drawRect(QRect(left + 1, top, colW - 2, wallH));
        }

        p.setPen(QColor(210, 210, 210));
        p.drawRect(QRect(cx0, cy0, colW * cols, H));
    }

private:
    std::array<int, 16> m_dist{};
};

class MainWindow final : public QMainWindow {
public:
    MainWindow() {
        m_mem.next = m_mem.p;
        m_mem.dist.fill(4);
        buildUi();
        refreshAll();
        resize(1180, 760);
        setWindowTitle("Human CPU Doom-like Qt PoC");
    }

private:
    Memory m_mem;

    QLabel* m_mapLabel = nullptr;
    FrameBufferWidget* m_frame = nullptr;
    QTableWidget* m_regTable = nullptr;
    QTableWidget* m_distTable = nullptr;
    QTableWidget* m_rayTable = nullptr;
    QTextEdit* m_log = nullptr;
    QComboBox* m_actionBox = nullptr;

    QSpinBox* m_nextX = nullptr;
    QSpinBox* m_nextY = nullptr;
    QSpinBox* m_nextDir = nullptr;

    void buildUi() {
        auto* central = new QWidget(this);
        auto* root = new QVBoxLayout(central);

        auto* top = new QHBoxLayout();
        root->addLayout(top, 1);

        auto* left = new QVBoxLayout();
        auto* middle = new QVBoxLayout();
        auto* right = new QVBoxLayout();
        top->addLayout(left, 0);
        top->addLayout(middle, 1);
        top->addLayout(right, 1);

        m_mapLabel = new QLabel();
        m_mapLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_mapLabel->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        m_mapLabel->setFrameShape(QFrame::StyledPanel);
        m_mapLabel->setMinimumWidth(230);
        left->addWidget(new QLabel("External memory: map"));
        left->addWidget(m_mapLabel);

        m_actionBox = new QComboBox();
        m_actionBox->addItem(".  none", ".");
        m_actionBox->addItem("w  forward", "w");
        m_actionBox->addItem("s  backward", "s");
        m_actionBox->addItem("a  turn left", "a");
        m_actionBox->addItem("d  turn right", "d");
        connect(m_actionBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
            m_mem.action = m_actionBox->currentData().toString().at(0);
            refreshRegisters();
            log(QString("ACTION <- %1").arg(m_mem.action));
        });
        left->addWidget(new QLabel("Input/action register"));
        left->addWidget(m_actionBox);

        auto* nextBox = new QGroupBox("Human-written NEXT registers");
        auto* nf = new QFormLayout(nextBox);
        m_nextX = makeSpin(0, 99);
        m_nextY = makeSpin(0, 99);
        m_nextDir = makeSpin(0, 3);
        nf->addRow("NEXT.X", m_nextX);
        nf->addRow("NEXT.Y", m_nextY);
        nf->addRow("NEXT.DIR", m_nextDir);
        connect(m_nextX, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v){ m_mem.next.x = v; refreshRegisters(); });
        connect(m_nextY, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v){ m_mem.next.y = v; refreshRegisters(); });
        connect(m_nextDir, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v){ m_mem.next.dir = v; refreshRegisters(); refreshRayTable(); });
        left->addWidget(nextBox);

        auto* buttons = new QGridLayout();
        addButton(buttons, 0, 0, "Validate", [this]{ validateHumanWork(); });
        addButton(buttons, 0, 1, "Hint NEXT", [this]{ hintNext(); });
        addButton(buttons, 1, 0, "Hint DIST", [this]{ hintDist(); });
        addButton(buttons, 1, 1, "Auto fill", [this]{ autoFill(); });
        addButton(buttons, 2, 0, "Commit", [this]{ commit(); });
        addButton(buttons, 2, 1, "Reset", [this]{ reset(); });
        left->addLayout(buttons);
        left->addStretch(1);

        m_regTable = new QTableWidget(7, 2);
        m_regTable->setHorizontalHeaderLabels({"Register", "Value"});
        m_regTable->horizontalHeader()->setStretchLastSection(true);
        m_regTable->verticalHeader()->hide();
        m_regTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_regTable->setMaximumHeight(230);
        middle->addWidget(new QLabel("Registers / RAM view"));
        middle->addWidget(m_regTable);

        m_distTable = new QTableWidget(2, 16);
        QStringList headers;
        for (int i = 0; i < 16; ++i) headers << QString::number(i);
        m_distTable->setHorizontalHeaderLabels(headers);
        m_distTable->setVerticalHeaderLabels({"DIST", "Expected"});
        for (int c = 0; c < 16; ++c) {
            auto* item = new QTableWidgetItem(QString::number(m_mem.dist[(size_t)c]));
            item->setTextAlignment(Qt::AlignCenter);
            m_distTable->setItem(0, c, item);
            auto* exp = new QTableWidgetItem("?");
            exp->setTextAlignment(Qt::AlignCenter);
            exp->setFlags(exp->flags() & ~Qt::ItemIsEditable);
            m_distTable->setItem(1, c, exp);
            m_distTable->setColumnWidth(c, 42);
        }
        connect(m_distTable, &QTableWidget::cellChanged, this, [this](int row, int col) {
            if (row != 0 || m_updating) return;
            bool ok = false;
            int v = m_distTable->item(row, col)->text().toInt(&ok);
            if (ok) m_mem.dist[(size_t)col] = std::clamp(v, 1, 9);
            refreshFrame();
        });
        middle->addWidget(new QLabel("Human-written VRAM: distance buffer"));
        middle->addWidget(m_distTable);

        m_rayTable = new QTableWidget(16, 4);
        m_rayTable->setHorizontalHeaderLabels({"Column", "Local ray", "World dx", "World dy"});
        m_rayTable->horizontalHeader()->setStretchLastSection(true);
        m_rayTable->verticalHeader()->hide();
        m_rayTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        middle->addWidget(new QLabel("Micro-instruction list: rays to compute"));
        middle->addWidget(m_rayTable, 1);

        m_frame = new FrameBufferWidget();
        right->addWidget(new QLabel("Framebuffer / display"));
        right->addWidget(m_frame, 1);

        m_log = new QTextEdit();
        m_log->setReadOnly(true);
        m_log->setMaximumHeight(170);
        root->addWidget(new QLabel("Execution log"));
        root->addWidget(m_log);

        setCentralWidget(central);
    }

    bool m_updating = false;

    static QSpinBox* makeSpin(int lo, int hi) {
        auto* s = new QSpinBox();
        s->setRange(lo, hi);
        return s;
    }

    template<class Fn>
    void addButton(QGridLayout* layout, int r, int c, const QString& text, Fn fn) {
        auto* b = new QPushButton(text);
        connect(b, &QPushButton::clicked, this, fn);
        layout->addWidget(b, r, c);
    }

    void refreshAll() {
        refreshMap();
        refreshNextEditors();
        refreshRegisters();
        refreshDistTable(false);
        refreshRayTable();
        refreshFrame();
        log("Ready. Human CPU mode: choose action, compute NEXT, compute DIST, then Commit.");
    }

    void refreshMap() {
        QString text;
        static const QChar icon[4] = {'^', '>', 'v', '<'};
        text += "Legend: # wall, . empty\n\n";
        for (int y = 0; y < (int)kMap.size(); ++y) {
            for (int x = 0; x < kMap[(size_t)y].size(); ++x) {
                if (x == m_mem.p.x && y == m_mem.p.y) text += icon[m_mem.p.dir];
                else text += kMap[(size_t)y][x];
            }
            text += '\n';
        }
        m_mapLabel->setText(text);
    }

    void refreshNextEditors() {
        QSignalBlocker bx(m_nextX), by(m_nextY), bd(m_nextDir);
        m_nextX->setValue(m_mem.next.x);
        m_nextY->setValue(m_mem.next.y);
        m_nextDir->setValue(m_mem.next.dir);
    }

    void setRegRow(int row, const QString& name, const QString& value) {
        auto* a = new QTableWidgetItem(name);
        auto* b = new QTableWidgetItem(value);
        a->setFlags(a->flags() & ~Qt::ItemIsEditable);
        b->setFlags(b->flags() & ~Qt::ItemIsEditable);
        m_regTable->setItem(row, 0, a);
        m_regTable->setItem(row, 1, b);
    }

    void refreshRegisters() {
        setRegRow(0, "P.X", QString::number(m_mem.p.x));
        setRegRow(1, "P.Y", QString::number(m_mem.p.y));
        setRegRow(2, "P.DIR", QString("%1 (%2)").arg(kDirName[m_mem.p.dir]).arg(m_mem.p.dir));
        setRegRow(3, "ACTION", QString(m_mem.action));
        setRegRow(4, "NEXT.X", QString::number(m_mem.next.x));
        setRegRow(5, "NEXT.Y", QString::number(m_mem.next.y));
        setRegRow(6, "NEXT.DIR", QString("%1 (%2)").arg(kDirName[std::clamp(m_mem.next.dir, 0, 3)]).arg(m_mem.next.dir));
        m_regTable->resizeColumnsToContents();
    }

    void refreshDistTable(bool showExpected) {
        m_updating = true;
        for (int c = 0; c < 16; ++c) {
            m_distTable->item(0, c)->setText(QString::number(m_mem.dist[(size_t)c]));
            if (showExpected) m_distTable->item(1, c)->setText(QString::number(autoDistance(m_mem.next, c)));
            else m_distTable->item(1, c)->setText("?");
        }
        m_updating = false;
    }

    void refreshRayTable() {
        for (int c = 0; c < 16; ++c) {
            RayDir local = localRayForColumn(c);
            RayDir world = rotateRay(local, std::clamp(m_mem.next.dir, 0, 3));
            setRayCell(c, 0, QString::number(c));
            setRayCell(c, 1, local.name);
            setRayCell(c, 2, QString::number(world.dx));
            setRayCell(c, 3, QString::number(world.dy));
        }
        m_rayTable->resizeColumnsToContents();
    }

    void setRayCell(int r, int c, const QString& text) {
        auto* item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_rayTable->setItem(r, c, item);
    }

    void refreshFrame() {
        m_frame->setDistances(m_mem.dist);
    }

    void log(const QString& msg) {
        m_log->append(QTime::currentTime().toString("HH:mm:ss") + "  " + msg);
    }

    void validateHumanWork() {
        Player n = autoNext(m_mem.p, m_mem.action);
        bool nextOk = (n.x == m_mem.next.x && n.y == m_mem.next.y && n.dir == m_mem.next.dir);
        int distErrors = 0;
        for (int c = 0; c < 16; ++c) {
            if (m_mem.dist[(size_t)c] != autoDistance(m_mem.next, c)) ++distErrors;
        }
        refreshDistTable(true);
        if (nextOk && distErrors == 0) log("[OK] NEXT and DIST match hidden reference.");
        else log(QString("[MISMATCH] NEXT=%1, DIST errors=%2. Expected row is now visible.")
                 .arg(nextOk ? "OK" : "wrong")
                 .arg(distErrors));
    }

    void hintNext() {
        Player n = autoNext(m_mem.p, m_mem.action);
        log(QString("Hint NEXT: x=%1 y=%2 dir=%3 (%4)")
            .arg(n.x).arg(n.y).arg(kDirName[n.dir]).arg(n.dir));
    }

    void hintDist() {
        refreshDistTable(true);
        QString s = "Hint DIST:";
        for (int c = 0; c < 16; ++c) s += " " + QString::number(autoDistance(m_mem.next, c));
        log(s);
    }

    void autoFill() {
        m_mem.next = autoNext(m_mem.p, m_mem.action);
        for (int c = 0; c < 16; ++c) m_mem.dist[(size_t)c] = autoDistance(m_mem.next, c);
        refreshNextEditors();
        refreshRegisters();
        refreshDistTable(false);
        refreshRayTable();
        refreshFrame();
        log("Auto-filled NEXT and DIST. This is debug mode, not pure human-CPU mode.");
    }

    void commit() {
        if (wall(m_mem.next.x, m_mem.next.y)) {
            log("Commit rejected: NEXT points to a wall/outside map.");
            return;
        }
        if (m_mem.next.dir < 0 || m_mem.next.dir > 3) {
            log("Commit rejected: NEXT.DIR must be 0..3.");
            return;
        }
        m_mem.p = m_mem.next;
        refreshMap();
        refreshRegisters();
        refreshRayTable();
        refreshFrame();
        log(QString("Committed P := NEXT. Player is now (%1,%2) dir=%3.")
            .arg(m_mem.p.x).arg(m_mem.p.y).arg(kDirName[m_mem.p.dir]));
    }

    void reset() {
        m_mem = Memory{};
        m_mem.next = m_mem.p;
        m_mem.dist.fill(4);
        m_actionBox->setCurrentIndex(0);
        refreshAll();
        log("Reset memory.");
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
