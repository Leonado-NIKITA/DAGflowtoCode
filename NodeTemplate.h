/**
 * @file NodeTemplate.h
 * @brief 节点模板类头文件，定义自定义节点的模板数据结构
 * @author
 * @version 1.0.0
 * @date 2024
 */

#ifndef NODETEMPLATE_H
#define NODETEMPLATE_H

#include <QString>
#include <QColor>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @class NodeTemplate
 * @brief 节点模板类，定义节点的类型、外观和默认属性
 *
 * 该类用于存储节点模板信息，支持：
 * - 节点类型标识和显示名称
 * - 节点颜色自定义
 * - 输入/输出端口配置
 * - 默认参数设置
 * - 模板的序列化和反序列化
 */
class NodeTemplate
{
public:
    /**
     * @brief 默认构造函数
     */
    NodeTemplate();

    /**
     * @brief 带参数的构造函数
     * @param typeId 节点类型标识符（唯一）
     * @param displayName 节点显示名称
     * @param category 节点所属分类
     * @param color 节点颜色
     */
    NodeTemplate(const QString &typeId, const QString &displayName,
                 const QString &category, const QColor &color);

    // 基本属性访问器
    QString getTypeId() const { return m_typeId; }
    QString getDisplayName() const { return m_displayName; }
    QString getCategory() const { return m_category; }
    QColor getColor() const { return m_color; }
    QString getDescription() const { return m_description; }
    QStringList getDefaultParameters() const { return m_defaultParameters; }
    int getInputPortCount() const { return m_inputPortCount; }
    int getOutputPortCount() const { return m_outputPortCount; }
    bool isBuiltIn() const { return m_isBuiltIn; }

    // 基本属性设置器
    void setTypeId(const QString &typeId) { m_typeId = typeId; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setCategory(const QString &category) { m_category = category; }
    void setColor(const QColor &color) { m_color = color; }
    void setDescription(const QString &description) { m_description = description; }
    void setDefaultParameters(const QStringList &params) { m_defaultParameters = params; }
    void setInputPortCount(int count) { m_inputPortCount = count; }
    void setOutputPortCount(int count) { m_outputPortCount = count; }
    void setBuiltIn(bool builtIn) { m_isBuiltIn = builtIn; }

    /**
     * @brief 将模板数据转换为JSON对象
     * @return 包含模板信息的JSON对象
     */
    QJsonObject toJson() const;

    /**
     * @brief 从JSON对象创建模板实例
     * @param json 包含模板信息的JSON对象
     * @return 新创建的模板对象
     */
    static NodeTemplate fromJson(const QJsonObject &json);

    /**
     * @brief 检查模板是否有效
     * @return 如果模板有效返回true
     */
    bool isValid() const;

private:
    QString m_typeId;              ///< 节点类型标识符（唯一）
    QString m_displayName;         ///< 节点显示名称
    QString m_category;            ///< 节点所属分类
    QColor m_color;                ///< 节点颜色
    QString m_description;         ///< 节点描述
    QStringList m_defaultParameters; ///< 默认参数列表
    int m_inputPortCount;          ///< 输入端口数量
    int m_outputPortCount;         ///< 输出端口数量
    bool m_isBuiltIn;              ///< 是否为内置节点
};

#endif // NODETEMPLATE_H

