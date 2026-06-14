#include "Config.h"
#include <QFile>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QDir>

Config& Config::instance() {
    static Config cfg;
    return cfg;
}

Config::Config() {
    load();
}

QString Config::filePath() const {
    return QCoreApplication::applicationDirPath() + "/touchtools.json";
}

void Config::load() {
    QFile f(filePath());
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        m_obj = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
    }

    bool dirty = false;
    auto def = [&](const char* key, int val) {
        if (!m_obj.contains(key)) { m_obj[key] = val; dirty = true; }
    };
    def("toolbar_opacity", 140);
    def("toolbar_icon_size", 28);
    def("toolbar_button_size", 44);
    def("control_opacity", 160);
    def("control_font_size", 22);
    def("control_btn_size", 44);
    def("control_edge_offset", 4);
    def("usb_popup_duration", 5000);

    if (!m_obj.contains("usb_enabled")) { m_obj["usb_enabled"] = true; dirty = true; }

    if (m_obj.contains("control_width"))  { m_obj.remove("control_width");  dirty = true; }
    if (m_obj.contains("control_height")) { m_obj.remove("control_height"); dirty = true; }

    if (dirty) save();
}

void Config::reload() {
    m_obj = QJsonObject();
    load();
}

void Config::save() {
    QFile f(filePath());
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(m_obj).toJson());
        f.close();
    }
}

int Config::toolbarOpacity()    const { return m_obj["toolbar_opacity"].toInt(); }
int Config::toolbarIconSize()    const { return m_obj["toolbar_icon_size"].toInt(); }
int Config::toolbarButtonSize()  const { return m_obj["toolbar_button_size"].toInt(); }
int Config::controlOpacity()     const { return m_obj["control_opacity"].toInt(); }
int Config::controlFontSize()    const { return m_obj["control_font_size"].toInt(); }
int Config::controlBtnSize()     const { return m_obj["control_btn_size"].toInt(); }
int Config::controlEdgeOffset()  const { return m_obj["control_edge_offset"].toInt(); }
bool Config::usbEnabled()         const { return m_obj["usb_enabled"].toBool(); }
int Config::usbPopupDuration()    const { return m_obj["usb_popup_duration"].toInt(); }
