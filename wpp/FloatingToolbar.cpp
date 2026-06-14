#include "FloatingToolbar.h"
#include "Config.h"
#include "KeySimulator.h"
#include <QHBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QPainter>
#include <QImage>
#include <QCoreApplication>
#include <QDir>
#include <cstdio>

#define LOG(fmt, ...) do { \
    fprintf(stderr, "[FloatingToolbar] " fmt "\n", ##__VA_ARGS__); \
    fflush(stderr); \
} while(0)

static QIcon loadIcon(const QString &name) {
    QDir dir(QCoreApplication::applicationDirPath());
    QString path = dir.absoluteFilePath("ico/" + name + ".png");
    if (!dir.exists("ico/" + name + ".png"))
        path = dir.absoluteFilePath("../ico/" + name + ".png");
    QImage img(path);
    if (img.isNull())
        return QIcon();
    img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < img.height(); ++y) {
        auto *p = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            int r = qRed(p[x]), g = qGreen(p[x]), b = qBlue(p[x]);
            if (r > 200 && g > 200 && b > 200) {
                p[x] = qRgba(0, 0, 0, 0);
            } else {
                int a = qAlpha(p[x]);
                p[x] = qRgba(255, 255, 255, a);
            }
        }
    }
    return QIcon(QPixmap::fromImage(img));
}

FloatingToolbar::FloatingToolbar(QWidget *parent)
    : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    QHBoxLayout *layout = new QHBoxLayout(this);

    m_mouseBtn = new QPushButton;
    m_mouseBtn->setIcon(loadIcon("mouse"));
    m_penBtn = new QPushButton;
    m_penBtn->setIcon(loadIcon("pen"));
    m_eraserBtn = new QPushButton;
    m_eraserBtn->setIcon(loadIcon("eraser"));
    m_exitBtn = new QPushButton;
    m_exitBtn->setIcon(loadIcon("exit"));

    layout->addWidget(m_mouseBtn);
    layout->addWidget(m_penBtn);
    layout->addWidget(m_eraserBtn);
    layout->addStretch();
    layout->addWidget(m_exitBtn);

    connect(m_mouseBtn, &QPushButton::clicked, this, &FloatingToolbar::onMouse);
    connect(m_penBtn, &QPushButton::clicked, this, &FloatingToolbar::onPen);
    connect(m_eraserBtn, &QPushButton::clicked, this, &FloatingToolbar::onEraser);
    connect(m_exitBtn, &QPushButton::clicked, this, &FloatingToolbar::onExit);

    m_mouseBtn->setAttribute(Qt::WA_AcceptTouchEvents);
    m_penBtn->setAttribute(Qt::WA_AcceptTouchEvents);
    m_eraserBtn->setAttribute(Qt::WA_AcceptTouchEvents);
    m_exitBtn->setAttribute(Qt::WA_AcceptTouchEvents);

    applyConfig();
    hide();

    m_timer.setInterval(500);
    connect(&m_timer, &QTimer::timeout, this, &FloatingToolbar::refreshLoop);
    m_timer.start();

    m_pulseTimer.setInterval(30000);
    connect(&m_pulseTimer, &QTimer::timeout, this, &FloatingToolbar::applyCurrentTool);
    m_pulseTimer.start();

    selectTool(Tool::Mouse);
}

void FloatingToolbar::applyConfig() {
    auto &cfg = Config::instance();
    int bs = cfg.toolbarButtonSize();
    int mg = bs / 8;
    int sp = mg;
    QSize is(cfg.toolbarIconSize(), cfg.toolbarIconSize());
    setFixedSize(bs * 4 + sp * 3 + mg * 2 + bs, bs + mg * 2);
    layout()->setContentsMargins(mg, mg, mg, mg);
    static_cast<QHBoxLayout*>(layout())->setSpacing(sp);

    m_mouseBtn->setIconSize(is);
    m_mouseBtn->setFixedSize(bs, bs);
    m_penBtn->setIconSize(is);
    m_penBtn->setFixedSize(bs, bs);
    m_eraserBtn->setIconSize(is);
    m_eraserBtn->setFixedSize(bs, bs);
    m_exitBtn->setIconSize(is);
    m_exitBtn->setFixedSize(bs, bs);

    updateButtonStyles();
    reposition();
}

void FloatingToolbar::setKeepAlive(bool on) {
    m_keepAlive = on;
}

void FloatingToolbar::reposition() {
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    int x = screenRect.center().x() - width() / 2;
    int y = screenRect.bottom() - height() - 8;
    move(x, y);
}

void FloatingToolbar::showEvent(QShowEvent *event) {
    reposition();
    QWidget::showEvent(event);
}

void FloatingToolbar::refreshLoop() {
    if (m_keepAlive) return;
    if (KeySimulator::isSlideShowActive()) {
        if (isHidden()) {
            show();
            raise();
        }
    } else {
        if (isVisible()) hide();
    }
}

void FloatingToolbar::onMouse()  { LOG("mouse click");  selectTool(Tool::Mouse); }
void FloatingToolbar::onPen()    { LOG("pen click");    selectTool(Tool::Pen); }
void FloatingToolbar::onEraser() { LOG("eraser click"); selectTool(Tool::Eraser); }
void FloatingToolbar::onExit()   { LOG("exit click");   KeySimulator::exitSlideShow(); }

void FloatingToolbar::selectTool(Tool t) {
    m_selectedTool = t;
    updateButtonStyles();
    applyCurrentTool();
}

void FloatingToolbar::applyCurrentTool() {
    switch (m_selectedTool) {
    case Tool::Mouse:  KeySimulator::mouseMode();  break;
    case Tool::Pen:    KeySimulator::penMode();    break;
    case Tool::Eraser: KeySimulator::eraserMode(); break;
    }
}

void FloatingToolbar::updateButtonStyles() {
    auto &cfg = Config::instance();
    int op = cfg.toolbarOpacity();

    auto toolStyle = [&](bool selected) {
        if (selected)
            return QString("QPushButton { background: rgba(0, 120, 215, 200); border: 1px solid rgba(255, 255, 255, 100); border-radius: 8px; }");
        return QString(
            "QPushButton {"
                "background: rgba(0, 0, 0, %1);"
                "border: 1px solid rgba(255, 255, 255, 40);"
                "border-radius: 8px;"
            "}"
            "QPushButton:hover {"
                "background: rgba(0, 0, 0, 200);"
                "border: 1px solid rgba(255, 255, 255, 100);"
            "}"
        ).arg(op);
    };

    m_mouseBtn->setStyleSheet(toolStyle(m_selectedTool == Tool::Mouse));
    m_penBtn->setStyleSheet(toolStyle(m_selectedTool == Tool::Pen));
    m_eraserBtn->setStyleSheet(toolStyle(m_selectedTool == Tool::Eraser));

    QString exitStyle = QString(
        "QPushButton {"
            "background: rgba(180, 30, 30, %1);"
            "border: 1px solid rgba(255, 80, 80, 60);"
            "border-radius: 8px;"
        "}"
        "QPushButton:hover {"
            "background: rgba(200, 40, 40, 220);"
            "border: 1px solid rgba(255, 100, 100, 120);"
        "}"
    ).arg(qMin(255, op + 20));
    m_exitBtn->setStyleSheet(exitStyle);
}
