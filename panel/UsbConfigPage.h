#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <QJsonObject>

class UsbConfigPage : public QWidget {
    Q_OBJECT
public:
    explicit UsbConfigPage(QWidget *parent = nullptr);
    void setActive(bool active);

private slots:
    void onStart();
    void onStop();
    void onSave();

private:
    void loadCfg();
    void saveCfg();
    void updateStatus();
    QWidget* makeSlider(const QString &label, const QString &key, int min, int max, int val);

    QLabel *m_statusIcon;
    QLabel *m_statusText;
    QPushButton *m_startBtn;
    QPushButton *m_stopBtn;
    QPushButton *m_saveBtn;
    QCheckBox *m_enabledCb;
    QTimer *m_timer;
    QJsonObject m_cfg;

    struct Entry { QString key; QSlider *sl; QSpinBox *sp; };
    QList<Entry> m_entries;
};
