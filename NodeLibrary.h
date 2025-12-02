/**
 * @file NodeLibrary.h
 * @brief 节点库类头文件，管理所有节点模板
 * @author
 * @version 1.0.0
 * @date 2024
 */

#ifndef NODELIBRARY_H
#define NODELIBRARY_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QString>
#include "NodeTemplate.h"

/**
 * @class NodeLibrary
 * @brief 节点库类，管理所有可用的节点模板
 *
 * 该类负责：
 * - 管理内置和自定义节点模板
 * - 节点库的保存和加载
 * - 按分类组织节点
 */
class NodeLibrary : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return 节点库单例指针
     */
    static NodeLibrary* instance();

    /**
     * @brief 获取所有节点模板
     * @return 节点模板列表
     */
    QList<NodeTemplate> getAllTemplates() const;

    /**
     * @brief 根据类型ID获取节点模板
     * @param typeId 节点类型ID
     * @return 节点模板，如果不存在则返回无效模板
     */
    NodeTemplate getTemplate(const QString &typeId) const;

    /**
     * @brief 获取所有分类
     * @return 分类名称列表
     */
    QStringList getCategories() const;

    /**
     * @brief 获取指定分类下的所有模板
     * @param category 分类名称
     * @return 该分类下的节点模板列表
     */
    QList<NodeTemplate> getTemplatesByCategory(const QString &category) const;

    /**
     * @brief 添加新的节点模板
     * @param tmpl 要添加的节点模板
     * @return 如果添加成功返回true
     */
    bool addTemplate(const NodeTemplate &tmpl);

    /**
     * @brief 更新现有的节点模板
     * @param tmpl 要更新的节点模板
     * @return 如果更新成功返回true
     */
    bool updateTemplate(const NodeTemplate &tmpl);

    /**
     * @brief 删除节点模板
     * @param typeId 要删除的节点类型ID
     * @return 如果删除成功返回true
     */
    bool removeTemplate(const QString &typeId);

    /**
     * @brief 检查节点类型ID是否已存在
     * @param typeId 节点类型ID
     * @return 如果存在返回true
     */
    bool hasTemplate(const QString &typeId) const;

    /**
     * @brief 将节点库保存到文件
     * @param filePath 文件路径
     * @return 如果保存成功返回true
     */
    bool saveToFile(const QString &filePath) const;

    /**
     * @brief 从文件加载节点库
     * @param filePath 文件路径
     * @return 如果加载成功返回true
     */
    bool loadFromFile(const QString &filePath);

    /**
     * @brief 将自定义节点保存到文件（不包含内置节点）
     * @param filePath 文件路径
     * @return 如果保存成功返回true
     */
    bool saveCustomToFile(const QString &filePath) const;

    /**
     * @brief 从文件加载自定义节点（追加到现有库）
     * @param filePath 文件路径
     * @return 如果加载成功返回true
     */
    bool loadCustomFromFile(const QString &filePath);

    /**
     * @brief 重置为默认内置节点
     */
    void resetToDefaults();

    /**
     * @brief 获取默认节点库文件路径
     * @return 默认文件路径
     */
    QString getDefaultLibraryPath() const;

signals:
    /**
     * @brief 节点库发生变化时发出的信号
     */
    void libraryChanged();

    /**
     * @brief 模板被添加时发出的信号
     * @param typeId 添加的模板类型ID
     */
    void templateAdded(const QString &typeId);

    /**
     * @brief 模板被更新时发出的信号
     * @param typeId 更新的模板类型ID
     */
    void templateUpdated(const QString &typeId);

    /**
     * @brief 模板被删除时发出的信号
     * @param typeId 删除的模板类型ID
     */
    void templateRemoved(const QString &typeId);

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    explicit NodeLibrary(QObject *parent = nullptr);

    /**
     * @brief 初始化内置节点模板
     */
    void initBuiltInTemplates();

    static NodeLibrary *s_instance;           ///< 单例实例
    QMap<QString, NodeTemplate> m_templates;  ///< 节点模板映射表
};

#endif // NODELIBRARY_H

