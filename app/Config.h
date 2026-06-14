#pragma once
#include <QString>
#include <QJsonObject>

class Config {
public:
    static Config& instance();

    int toolbarOpacity() const;
    int toolbarIconSize() const;
    int toolbarButtonSize() const;
    int controlOpacity() const;
    int controlFontSize() const;
    int controlBtnSize() const;
    int controlEdgeOffset() const;
    bool usbEnabled() const;
    int usbPopupDuration() const;

    void save();
    void reload();

private:
    Config();
    void load();
    QString filePath() const;
    QJsonObject m_obj;
};
