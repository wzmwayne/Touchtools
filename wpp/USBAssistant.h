#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QAbstractNativeEventFilter>

class USBAssistant : public QWidget, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
    explicit USBAssistant(QWidget *parent = nullptr);
    ~USBAssistant() override;

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private slots:
    void onOpenDrive();
    void deliverPopup(const QString &drivePath);

private:
    QString volumeNameForDrive(const QString &drivePath);
    static char driveFromMask(unsigned long unitmask);

    QLabel *m_titleLabel;
    QLabel *m_infoLabel;
    QPushButton *m_openBtn;
    QTimer m_hideTimer;
    QString m_drivePath;
};
