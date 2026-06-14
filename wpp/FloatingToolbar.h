#pragma once
#include <QWidget>
#include <QPushButton>
#include <QTimer>

class FloatingToolbar : public QWidget {
    Q_OBJECT
public:
    explicit FloatingToolbar(QWidget *parent = nullptr);
    void applyConfig();
    void setKeepAlive(bool on);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onMouse();
    void onPen();
    void onEraser();
    void onExit();
    void refreshLoop();

private:
    enum class Tool { Mouse, Pen, Eraser };
    void reposition();
    void selectTool(Tool t);
    void applyCurrentTool();
    void updateButtonStyles();

    QPushButton *m_mouseBtn;
    QPushButton *m_penBtn;
    QPushButton *m_eraserBtn;
    QPushButton *m_exitBtn;
    QTimer m_timer;
    QTimer m_pulseTimer;
    Tool m_selectedTool = Tool::Mouse;
    bool m_keepAlive = false;
};
