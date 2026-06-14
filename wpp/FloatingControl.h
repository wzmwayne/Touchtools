#pragma once
#include <QWidget>
#include <QPushButton>
#include <QTimer>

class FloatingControl : public QWidget {
    Q_OBJECT
public:
    enum Side { Left, Right };
    explicit FloatingControl(Side side, QWidget *parent = nullptr);
    void applyConfig();
    void setKeepAlive(bool on);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onPrev();
    void onNext();
    void refreshLoop();

private:
    void reposition();

    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    Side m_side;
    QTimer m_timer;
    bool m_keepAlive = false;
};
