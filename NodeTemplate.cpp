/**
 * @file NodeTemplate.cpp
 * @brief 节点模板类实现文件
 * @author
 * @version 1.0.0
 * @date 2024
 */

#include "NodeTemplate.h"

/**
 * @brief 默认构造函数
 */
NodeTemplate::NodeTemplate()
    : m_typeId("")
    , m_displayName("")
    , m_category("自定义")
    , m_color(Qt::gray)
    , m_description("")
    , m_inputPortCount(1)
    , m_outputPortCount(1)
    , m_isBuiltIn(false)
{
}

/**
 * @brief 带参数的构造函数
 */
NodeTemplate::NodeTemplate(const QString &typeId, const QString &displayName,
                           const QString &category, const QColor &color)
    : m_typeId(typeId)
    , m_displayName(displayName)
    , m_category(category)
    , m_color(color)
    , m_description("")
    , m_inputPortCount(1)
    , m_outputPortCount(1)
    , m_isBuiltIn(false)
{
}

/**
 * @brief 将模板数据转换为JSON对象
 */
QJsonObject NodeTemplate::toJson() const
{
    QJsonObject json;
    json["typeId"] = m_typeId;
    json["displayName"] = m_displayName;
    json["category"] = m_category;
    json["color"] = m_color.name();  // 颜色转为十六进制字符串
    json["description"] = m_description;
    json["inputPortCount"] = m_inputPortCount;
    json["outputPortCount"] = m_outputPortCount;
    json["isBuiltIn"] = m_isBuiltIn;

    // 参数列表转为JSON数组
    QJsonArray paramsArray;
    for (const QString &param : m_defaultParameters) {
        paramsArray.append(param);
    }
    json["defaultParameters"] = paramsArray;

    return json;
}

/**
 * @brief 从JSON对象创建模板实例
 */
NodeTemplate NodeTemplate::fromJson(const QJsonObject &json)
{
    NodeTemplate tmpl;
    tmpl.m_typeId = json["typeId"].toString();
    tmpl.m_displayName = json["displayName"].toString();
    tmpl.m_category = json["category"].toString("自定义");
    tmpl.m_color = QColor(json["color"].toString("#808080"));
    tmpl.m_description = json["description"].toString();
    tmpl.m_inputPortCount = json["inputPortCount"].toInt(1);
    tmpl.m_outputPortCount = json["outputPortCount"].toInt(1);
    tmpl.m_isBuiltIn = json["isBuiltIn"].toBool(false);

    // 解析参数列表
    QJsonArray paramsArray = json["defaultParameters"].toArray();
    QStringList params;
    for (const QJsonValue &param : paramsArray) {
        params.append(param.toString());
    }
    tmpl.m_defaultParameters = params;

    return tmpl;
}

/**
 * @brief 检查模板是否有效
 */
bool NodeTemplate::isValid() const
{
    return !m_typeId.isEmpty() && !m_displayName.isEmpty();
}

