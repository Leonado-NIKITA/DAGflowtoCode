/**
 * @file NodeLibrary.cpp
 * @brief 节点库类实现文件
 * @author
 * @version 1.0.0
 * @date 2024
 */

#include "NodeLibrary.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

// 静态成员初始化
NodeLibrary* NodeLibrary::s_instance = nullptr;

/**
 * @brief 获取单例实例
 */
NodeLibrary* NodeLibrary::instance()
{
    if (!s_instance) {
        s_instance = new NodeLibrary();
    }
    return s_instance;
}

/**
 * @brief 私有构造函数
 */
NodeLibrary::NodeLibrary(QObject *parent)
    : QObject(parent)
{
    QString libraryPath = getDefaultLibraryPath();
    
    // 如果节点库文件存在，从文件加载
    if (QFile::exists(libraryPath)) {
        if (!loadFromFile(libraryPath)) {
            // 加载失败，使用默认节点并保存
            qWarning() << "加载节点库文件失败，使用默认配置";
            initBuiltInTemplates();
            saveToFile(libraryPath);
        }
    } else {
        // 节点库文件不存在，初始化默认节点并自动生成文件
        qDebug() << "节点库文件不存在，生成默认节点库:" << libraryPath;
        initBuiltInTemplates();
        saveToFile(libraryPath);
    }
}

/**
 * @brief 初始化默认节点模板
 * 
 * 所有节点都是可编辑的，不再区分内置和自定义
 */
void NodeLibrary::initBuiltInTemplates()
{
    // 信号源节点
    NodeTemplate signalSource("signal_source", "信号源", "信号处理", QColor(81, 207, 102));
    signalSource.setDescription("生成各种类型的信号源");
    signalSource.setInputPortCount(0);
    signalSource.setOutputPortCount(1);
    m_templates.insert(signalSource.getTypeId(), signalSource);

    // 滤波器节点
    NodeTemplate filter("filter", "滤波器", "信号处理", QColor(51, 154, 240));
    filter.setDescription("对信号进行滤波处理");
    filter.setInputPortCount(1);
    filter.setOutputPortCount(1);
    m_templates.insert(filter.getTypeId(), filter);

    // FFT变换节点
    NodeTemplate fft("fft", "FFT变换", "信号处理", QColor(204, 93, 232));
    fft.setDescription("对信号进行快速傅里叶变换");
    fft.setInputPortCount(1);
    fft.setOutputPortCount(1);
    m_templates.insert(fft.getTypeId(), fft);

    // 调制器节点
    NodeTemplate modulator("modulator", "调制器", "通信", QColor(252, 196, 25));
    modulator.setDescription("对信号进行调制");
    modulator.setInputPortCount(1);
    modulator.setOutputPortCount(1);
    m_templates.insert(modulator.getTypeId(), modulator);

    // 解调器节点
    NodeTemplate demodulator("demodulator", "解调器", "通信", QColor(255, 146, 43));
    demodulator.setDescription("对信号进行解调");
    demodulator.setInputPortCount(1);
    demodulator.setOutputPortCount(1);
    m_templates.insert(demodulator.getTypeId(), demodulator);

    // 输出节点
    NodeTemplate sink("sink", "输出", "通信", QColor(255, 107, 107));
    sink.setDescription("信号输出/显示节点");
    sink.setInputPortCount(1);
    sink.setOutputPortCount(0);
    m_templates.insert(sink.getTypeId(), sink);
}

/**
 * @brief 获取所有节点模板
 */
QList<NodeTemplate> NodeLibrary::getAllTemplates() const
{
    return m_templates.values();
}

/**
 * @brief 根据类型ID获取节点模板
 */
NodeTemplate NodeLibrary::getTemplate(const QString &typeId) const
{
    return m_templates.value(typeId, NodeTemplate());
}

/**
 * @brief 获取所有分类
 */
QStringList NodeLibrary::getCategories() const
{
    QSet<QString> categories;
    for (const NodeTemplate &tmpl : m_templates) {
        categories.insert(tmpl.getCategory());
    }
    return categories.values();
}

/**
 * @brief 获取指定分类下的所有模板
 */
QList<NodeTemplate> NodeLibrary::getTemplatesByCategory(const QString &category) const
{
    QList<NodeTemplate> result;
    for (const NodeTemplate &tmpl : m_templates) {
        if (tmpl.getCategory() == category) {
            result.append(tmpl);
        }
    }
    return result;
}

/**
 * @brief 添加新的节点模板
 */
bool NodeLibrary::addTemplate(const NodeTemplate &tmpl)
{
    if (!tmpl.isValid() || m_templates.contains(tmpl.getTypeId())) {
        return false;
    }

    m_templates.insert(tmpl.getTypeId(), tmpl);
    emit templateAdded(tmpl.getTypeId());
    emit libraryChanged();
    
    // 自动保存到文件
    saveToFile(getDefaultLibraryPath());
    
    return true;
}

/**
 * @brief 更新现有的节点模板
 * 
 * 所有节点都可以编辑
 */
bool NodeLibrary::updateTemplate(const NodeTemplate &tmpl)
{
    if (!tmpl.isValid() || !m_templates.contains(tmpl.getTypeId())) {
        return false;
    }

    m_templates[tmpl.getTypeId()] = tmpl;
    emit templateUpdated(tmpl.getTypeId());  // 发出模板更新信号
    emit libraryChanged();
    
    // 自动保存到文件
    saveToFile(getDefaultLibraryPath());
    
    return true;
}

/**
 * @brief 删除节点模板
 * 
 * 所有节点都可以删除
 */
bool NodeLibrary::removeTemplate(const QString &typeId)
{
    if (!m_templates.contains(typeId)) {
        return false;
    }

    m_templates.remove(typeId);
    emit templateRemoved(typeId);
    emit libraryChanged();
    
    // 自动保存到文件
    saveToFile(getDefaultLibraryPath());
    
    return true;
}

/**
 * @brief 检查节点类型ID是否已存在
 */
bool NodeLibrary::hasTemplate(const QString &typeId) const
{
    return m_templates.contains(typeId);
}

/**
 * @brief 将节点库保存到文件
 */
bool NodeLibrary::saveToFile(const QString &filePath) const
{
    QJsonObject root;
    root["version"] = "1.0";
    root["description"] = "节点库配置文件";

    QJsonArray templatesArray;
    for (const NodeTemplate &tmpl : m_templates) {
        templatesArray.append(tmpl.toJson());
    }
    root["templates"] = templatesArray;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法打开文件保存节点库:" << filePath;
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

/**
 * @brief 从文件加载节点库
 */
bool NodeLibrary::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件加载节点库:" << filePath;
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qWarning() << "节点库文件格式错误";
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray templatesArray = root["templates"].toArray();

    m_templates.clear();
    for (const QJsonValue &value : templatesArray) {
        NodeTemplate tmpl = NodeTemplate::fromJson(value.toObject());
        if (tmpl.isValid()) {
            m_templates.insert(tmpl.getTypeId(), tmpl);
        }
    }

    emit libraryChanged();
    return true;
}

/**
 * @brief 将自定义节点保存到文件
 */
bool NodeLibrary::saveCustomToFile(const QString &filePath) const
{
    QJsonObject root;
    root["version"] = "1.0";
    root["description"] = "自定义节点库配置文件";

    QJsonArray templatesArray;
    for (const NodeTemplate &tmpl : m_templates) {
        // 只保存自定义节点
        if (!tmpl.isBuiltIn()) {
            templatesArray.append(tmpl.toJson());
        }
    }
    root["templates"] = templatesArray;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法打开文件保存自定义节点库:" << filePath;
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

/**
 * @brief 从文件加载自定义节点
 */
bool NodeLibrary::loadCustomFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件加载自定义节点库:" << filePath;
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qWarning() << "自定义节点库文件格式错误";
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray templatesArray = root["templates"].toArray();

    for (const QJsonValue &value : templatesArray) {
        NodeTemplate tmpl = NodeTemplate::fromJson(value.toObject());
        if (tmpl.isValid() && !tmpl.isBuiltIn()) {
            // 如果已存在则更新，否则添加
            if (m_templates.contains(tmpl.getTypeId())) {
                m_templates[tmpl.getTypeId()] = tmpl;
            } else {
                m_templates.insert(tmpl.getTypeId(), tmpl);
            }
        }
    }

    emit libraryChanged();
    return true;
}

/**
 * @brief 重置为默认内置节点
 */
void NodeLibrary::resetToDefaults()
{
    m_templates.clear();
    initBuiltInTemplates();
    emit libraryChanged();
}

/**
 * @brief 获取默认节点库文件路径
 * 
 * 节点库文件存放在程序工作目录下
 */
QString NodeLibrary::getDefaultLibraryPath() const
{
    return QDir::currentPath() + "/node_library.json";
}

