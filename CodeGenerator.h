/**
 * @file CodeGenerator.h
 * @brief 代码生成器类头文件，负责将节点流程图转换为可执行代码
 * @author 
 * @version 1.0.0
 * @date 2024
 */

#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <QString>       // Qt字符串类
#include <QJsonObject>   // JSON对象类

/**
 * @class CodeGenerator
 * @brief 代码生成器类
 * 
 * 该类负责将节点编辑器中的流程图数据转换为可执行的代码。
 * 支持生成完整的C++代码文件，包括头文件、主函数等。
 */
class CodeGenerator
{
public:
    /**
     * @brief 构造函数
     */
    CodeGenerator();
    
    /**
     * @brief 生成代码的主函数
     * @param flowData 流程图的JSON数据
     * @return 生成的代码字符串
     */
    QString generateCode(const QJsonObject &flowData);
    
    /**
     * @brief 生成连接状态的JSON表示
     * @param flowData 标准流程图的JSON数据
     * @return 包含连接状态信息的JSON对象
     */
    QJsonObject generateConnectionStatus(const QJsonObject &flowData);
    
    /**
     * @brief 分析节点间的依赖关系
     * @param flowData 标准流程图的JSON数据
     * @return 依赖关系图
     */
    QMap<QString, QStringList> analyzeDependencies(const QJsonObject &flowData);
    
    /**
     * @brief 生成执行顺序
     * @param dependencies 依赖关系图
     * @return 按执行顺序排列的节点ID列表
     */
    QStringList generateExecutionOrder(const QMap<QString, QStringList> &dependencies);
    
    /**
     * @brief 根据标准JSON格式生成Python代码
     * @param flowData 标准流程图的JSON数据
     * @return 生成的Python代码字符串
     */
    QString generatePythonCode(const QJsonObject &flowData);
    
    /**
     * @brief 根据标准JSON格式生成配置文件
     * @param flowData 标准流程图的JSON数据
     * @return 配置文件内容
     */
    QString generateConfigFile(const QJsonObject &flowData);

private:
    /**
     * @brief 生成代码文件头部
     * @return 头部代码字符串
     */
    QString generateHeader();
    
    /**
     * @brief 生成单个节点的代码
     * @param node 节点的JSON数据
     * @return 节点代码字符串
     */
    QString generateNodeCode(const QJsonObject &node);
    
    /**
     * @brief 生成主函数代码
     * @param flowData 流程图的JSON数据
     * @return 主函数代码字符串
     */
    QString generateMainFunction(const QJsonObject &flowData);
    
    /**
     * @brief 生成代码文件尾部
     * @return 尾部代码字符串
     */
    QString generateFooter();
    
    /**
     * @brief 根据节点类型生成对应的变量声明
     * @param nodeType 节点类型
     * @param nodeName 节点名称
     * @return 变量声明代码
     */
    QString generateVariableDeclaration(const QString &nodeType, const QString &nodeName);
    
    /**
     * @brief 根据节点类型生成初始化代码
     * @param node 节点的JSON数据
     * @return 初始化代码
     */
    QString generateInitializationCode(const QJsonObject &node);
    
    /**
     * @brief 生成节点处理代码
     * @param nodeName 节点名称
     * @param nodeType 节点类型
     * @param dependencies 依赖列表
     * @return 处理代码字符串
     */
    QString generateNodeProcessingCode(const QString &nodeName, const QString &nodeType, const QStringList &dependencies);
};

#endif // CODEGENERATOR_H