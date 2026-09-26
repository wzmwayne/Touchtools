#pragma once
#include <QWidget>
#include <QListWidget>
#include <QStackedWidget>

class ConfigPanel : public QWidget {
    Q_OBJECT
public:
    explicit ConfigPanel(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onTabChanged(int index);

private:
    QWidget *m_titleBar;
    QListWidget *m_navList;
    QStackedWidget *m_stack;
    QPoint m_dragPos;
    bool m_dragging = false;
};
