#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QJsonObject>

class WppConfigPage : public QWidget {
    Q_OBJECT
public:
    explicit WppConfigPage(QWidget *parent = nullptr);

private slots:
    void onStart();
    void onStop();
    void onSave();
    void onAutoStartToggled(bool on);

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
    QCheckBox *m_autoStartCb;
    QCheckBox *m_usbCb;
    QTimer *m_timer;
    QJsonObject m_cfg;

    struct Entry { QString key; QSlider *sl; QSpinBox *sp; };
    QList<Entry> m_entries;
};
