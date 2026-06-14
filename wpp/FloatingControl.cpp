#include "FloatingControl.h"
#include "Config.h"
#include "KeySimulator.h"
#include <QVBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QGraphicsDropShadowEffect>
#include <cstdio>

#define LOG(fmt, ...) do { \
    fprintf(stderr, "[FloatingControl] " fmt "\n", ##__VA_ARGS__); \
    fflush(stderr); \
} while(0)

FloatingControl::FloatingControl(Side side, QWidget *parent)
    : QWidget(parent), m_side(side) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_prevBtn = new QPushButton("\u25b2");
    m_nextBtn = new QPushButton("\u25bc");

    m_prevBtn->setAttribute(Qt::WA_AcceptTouchEvents);
    m_nextBtn->setAttribute(Qt::WA_AcceptTouchEvents);

    layout->addStretch();
    layout->addWidget(m_prevBtn, 0, Qt::AlignCenter);
    layout->addWidget(m_nextBtn, 0, Qt::AlignCenter);
    layout->addStretch();

    connect(m_prevBtn, &QPushButton::clicked, this, &FloatingControl::onPrev);
    connect(m_nextBtn, &QPushButton::clicked, this, &FloatingControl::onNext);

    auto touchPressed = [this](QPushButton *btn) {
        connect(btn, &QPushButton::pressed, this, [this, btn]() {
            QString s = btn->styleSheet();
            s.replace("background: rgba(0, 0, 0,", "background: rgba(0, 120, 215,");
            btn->setStyleSheet(s);
        });
        connect(btn, &QPushButton::released, this, [this]() {
            applyConfig();
        });
    };
    touchPressed(m_prevBtn);
    touchPressed(m_nextBtn);

    applyConfig();
    hide();

    m_timer.setInterval(500);
    connect(&m_timer, &QTimer::timeout, this, &FloatingControl::refreshLoop);
    m_timer.start();
}

void FloatingControl::applyConfig() {
    auto &cfg = Config::instance();
    int s = cfg.controlBtnSize();
    setFixedSize(s + 20, s * 2 + 24);
    m_prevBtn->setFixedSize(s, s);
    m_nextBtn->setFixedSize(s, s);

    QString btnStyle = QString(
        "QPushButton {"
            "font-size: %1px;"
            "background: rgba(0, 0, 0, %2);"
            "color: white;"
            "border: 1px solid rgba(255, 255, 255, 60);"
            "border-radius: 12px;"
        "}"
        "QPushButton:hover {"
            "background: rgba(0, 0, 0, 200);"
            "border: 1px solid rgba(255, 255, 255, 120);"
        "}"
    ).arg(cfg.controlFontSize()).arg(cfg.controlOpacity());
    m_prevBtn->setStyleSheet(btnStyle);
    m_nextBtn->setStyleSheet(btnStyle);
    reposition();
}

void FloatingControl::setKeepAlive(bool on) {
    m_keepAlive = on;
}

void FloatingControl::reposition() {
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    int off = Config::instance().controlEdgeOffset();
    int y = screenRect.center().y() - height() / 2;
    int x = (m_side == Left)
        ? screenRect.left() + off
        : screenRect.right() - width() - off;
    move(x, y);
}

void FloatingControl::showEvent(QShowEvent *event) {
    reposition();
    QWidget::showEvent(event);
}

void FloatingControl::refreshLoop() {
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

void FloatingControl::onPrev() { LOG("prev slide"); KeySimulator::prevSlide(); }
void FloatingControl::onNext() { LOG("next slide"); KeySimulator::nextSlide(); }
