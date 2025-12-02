/**
 * @file CodeGenerator.cpp
 * @brief 代码生成器类实现文件
 * @author 
 * @version 1.0.0
 * @date 2024
 */

#include "CodeGenerator.h"
#include <QJsonArray>    // JSON数组类
#include <QJsonDocument> // JSON文档类
#include <QDateTime>    // 日期时间类
#include <QDebug>       // 调试输出类

/**
 * @brief 构造函数
 */
CodeGenerator::CodeGenerator()
{
}

/**
 * @brief 生成代码的主函数
 * @param flowData 流程图的JSON数据，包含节点和连接信息
 * @return 生成的JSON格式字符串，表达节点和连接关系
 * 
 * 输出格式包含：
 * - metadata: 元数据（标题、创建时间、版本）
 * - nodes: 节点数组（id、type、name、position、parameters）
 * - connections: 连接数组（from、to、fromPort、toPort）
 */
QString CodeGenerator::generateCode(const QJsonObject &flowData)
{
    QJsonObject output;
    
    // 1. 生成元数据
    QJsonObject metadata;
    if (flowData.contains("metadata")) {
        metadata = flowData["metadata"].toObject();
    } else {
        metadata["title"] = "可视化节点编辑器流程图";
        metadata["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        metadata["version"] = "1.0";
    }
    output["metadata"] = metadata;
    
    // 2. 生成节点数组（简化格式）
    QJsonArray nodesArray;
    QJsonArray sourceNodes = flowData["nodes"].toArray();
    for (const QJsonValue &nodeValue : sourceNodes) {
        QJsonObject sourceNode = nodeValue.toObject();
        QJsonObject node;
        
        node["id"] = sourceNode["id"];
        node["type"] = sourceNode["type"];
        node["name"] = sourceNode["name"];
        
        // 位置信息
        if (sourceNode.contains("position")) {
            node["position"] = sourceNode["position"];
        } else {
            // 兼容旧格式
            QJsonObject position;
            position["x"] = sourceNode["x"].toDouble();
            position["y"] = sourceNode["y"].toDouble();
            node["position"] = position;
        }
        
        // 参数
        if (sourceNode.contains("parameters")) {
            node["parameters"] = sourceNode["parameters"];
        } else {
            node["parameters"] = QJsonObject();
        }
        
        nodesArray.append(node);
    }
    output["nodes"] = nodesArray;
    
    // 3. 生成连接数组（简化格式）
    QJsonArray connectionsArray;
    QJsonArray sourceConnections = flowData["connections"].toArray();
    for (const QJsonValue &connValue : sourceConnections) {
        QJsonObject sourceConn = connValue.toObject();
        QJsonObject conn;
        
        conn["from"] = sourceConn["from"];
        conn["to"] = sourceConn["to"];
        
        // 可选：包含端口信息
        if (sourceConn.contains("fromPort")) {
            conn["fromPort"] = sourceConn["fromPort"];
        }
        if (sourceConn.contains("toPort")) {
            conn["toPort"] = sourceConn["toPort"];
        }
        
        connectionsArray.append(conn);
    }
    output["connections"] = connectionsArray;
    
    // 转换为格式化的JSON字符串
    QJsonDocument doc(output);
    return doc.toJson(QJsonDocument::Indented);
}

/**
 * @brief 生成连接状态的JSON表示
 * @param flowData 流程图的JSON数据
 * @return 包含连接状态信息的JSON对象
 */
QJsonObject CodeGenerator::generateConnectionStatus(const QJsonObject &flowData)
{
    QJsonObject status;
    
    // 基本元数据
    status["metadata"] = flowData["metadata"];
    status["analysis_timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    // 节点信息
    QJsonArray nodes = flowData["nodes"].toArray();
    QJsonObject nodeStatus;
    QMap<QString, QString> nodeIdToName;  // 节点ID到名称的映射
    
    for (const QJsonValue &nodeValue : nodes) {
        QJsonObject node = nodeValue.toObject();
        QString nodeId = node["id"].toString();
        QString nodeName = node["name"].toString();
        QString nodeType = node["type"].toString();
        
        nodeIdToName[nodeId] = nodeName;
        
        QJsonObject nodeInfo;
        nodeInfo["id"] = nodeId;
        nodeInfo["name"] = nodeName;
        nodeInfo["type"] = nodeType;
        nodeInfo["position"] = node["position"];  // 直接使用标准格式的position对象
        nodeInfo["parameters"] = node["parameters"];
        
        // 初始化连接状态
        nodeInfo["incoming_connections"] = QJsonArray();
        nodeInfo["outgoing_connections"] = QJsonArray();
        nodeInfo["predecessors"] = QJsonArray();
        nodeInfo["successors"] = QJsonArray();
        
        nodeStatus[nodeId] = nodeInfo;
    }
    
    // 连接关系分析 - 使用标准格式的from/to字段
    QJsonArray connections = flowData["connections"].toArray();
    QJsonArray connectionList;
    QMap<QString, QStringList> dependencies;
    
    // 初始化依赖关系
    for (const QJsonValue &nodeValue : nodes) {
        QString nodeId = nodeValue.toObject()["id"].toString();
        dependencies[nodeId] = QStringList();
    }
    
    for (const QJsonValue &connValue : connections) {
        QJsonObject conn = connValue.toObject();
        QString fromNodeId = conn["from"].toString();  // 使用标准格式的from字段
        QString toNodeId = conn["to"].toString();        // 使用标准格式的to字段
        
        // 构建连接详情
        QJsonObject connectionDetail;
        connectionDetail["id"] = QString("conn_%1_%2").arg(fromNodeId).arg(toNodeId);
        connectionDetail["from_node"] = QJsonObject{
            {"id", fromNodeId},
            {"name", nodeIdToName[fromNodeId]}
        };
        connectionDetail["to_node"] = QJsonObject{
            {"id", toNodeId},
            {"name", nodeIdToName[toNodeId]}
        };
        connectionDetail["connection_type"] = "data_flow";
        connectionDetail["status"] = "active";
        
        connectionList.append(connectionDetail);
        
        // 更新节点连接状态
        if (nodeStatus.contains(fromNodeId)) {
            QJsonObject fromNode = nodeStatus[fromNodeId].toObject();
            QJsonArray outgoing = fromNode["outgoing_connections"].toArray();
            outgoing.append(QJsonObject{
                {"target_id", toNodeId},
                {"target_name", nodeIdToName[toNodeId]},
                {"connection_id", connectionDetail["id"]}
            });
            fromNode["outgoing_connections"] = outgoing;
            
            QJsonArray successors = fromNode["successors"].toArray();
            successors.append(toNodeId);
            fromNode["successors"] = successors;
            
            nodeStatus[fromNodeId] = fromNode;
        }
        
        if (nodeStatus.contains(toNodeId)) {
            QJsonObject toNode = nodeStatus[toNodeId].toObject();
            QJsonArray incoming = toNode["incoming_connections"].toArray();
            incoming.append(QJsonObject{
                {"source_id", fromNodeId},
                {"source_name", nodeIdToName[fromNodeId]},
                {"connection_id", connectionDetail["id"]}
            });
            toNode["incoming_connections"] = incoming;
            
            QJsonArray predecessors = toNode["predecessors"].toArray();
            predecessors.append(fromNodeId);
            toNode["predecessors"] = predecessors;
            
            nodeStatus[toNodeId] = toNode;
        }
        
        // 构建依赖关系
        dependencies[toNodeId].append(fromNodeId);
    }
    
    status["nodes"] = nodeStatus;
    status["connections"] = connectionList;
    
    // 依赖关系分析
    QJsonObject depsObj;
    for (auto it = dependencies.begin(); it != dependencies.end(); ++it) {
        QJsonArray deps;
        for (const QString &dep : it.value()) {
            deps.append(dep);
        }
        depsObj[it.key()] = deps;
    }
    status["dependencies"] = depsObj;
    
    // 执行顺序分析
    QStringList executionOrder = generateExecutionOrder(dependencies);
    QJsonArray execOrderArray;
    for (const QString &nodeId : executionOrder) {
        execOrderArray.append(QJsonObject{
            {"id", nodeId},
            {"name", nodeIdToName[nodeId]}
        });
    }
    status["execution_order"] = execOrderArray;
    
    // 流程分析
    QJsonObject flowAnalysis;
    flowAnalysis["total_nodes"] = nodes.size();
    flowAnalysis["total_connections"] = connections.size();
    flowAnalysis["entry_nodes"] = QJsonArray();  // 没有前驱的节点
    flowAnalysis["exit_nodes"] = QJsonArray();   // 没有后继的节点
    
    for (auto it = nodeStatus.begin(); it != nodeStatus.end(); ++it) {
        QJsonObject nodeInfo = it.value().toObject();
        QJsonArray predecessors = nodeInfo["predecessors"].toArray();
        QJsonArray successors = nodeInfo["successors"].toArray();
        
        if (predecessors.isEmpty()) {
            flowAnalysis["entry_nodes"].toArray().append(nodeInfo["name"]);
        }
        if (successors.isEmpty()) {
            flowAnalysis["exit_nodes"].toArray().append(nodeInfo["name"]);
        }
    }
    
    status["flow_analysis"] = flowAnalysis;
    
    return status;
}

/**
 * @brief 分析节点间的依赖关系
 * @param flowData 流程图的JSON数据
 * @return 依赖关系图
 */
QMap<QString, QStringList> CodeGenerator::analyzeDependencies(const QJsonObject &flowData)
{
    QMap<QString, QStringList> dependencies;
    
    // 初始化所有节点的依赖列表
    QJsonArray nodes = flowData["nodes"].toArray();
    for (const QJsonValue &nodeValue : nodes) {
        QString nodeId = nodeValue.toObject()["id"].toString();
        dependencies[nodeId] = QStringList();
    }
    
    // 分析连接关系构建依赖 - 使用标准格式
    QJsonArray connections = flowData["connections"].toArray();
    for (const QJsonValue &connValue : connections) {
        QJsonObject conn = connValue.toObject();
        QString fromNodeId = conn["from"].toString();  // 标准格式使用from字段
        QString toNodeId = conn["to"].toString();        // 标准格式使用to字段
        
        dependencies[toNodeId].append(fromNodeId);
    }
    
    return dependencies;
}

/**
 * @brief 生成执行顺序
 * @param dependencies 依赖关系图
 * @return 按执行顺序排列的节点ID列表
 */
QStringList CodeGenerator::generateExecutionOrder(const QMap<QString, QStringList> &dependencies)
{
    QStringList executionOrder;
    QMap<QString, int> inDegree;
    
    // 计算每个节点的入度
    for (auto it = dependencies.begin(); it != dependencies.end(); ++it) {
        inDegree[it.key()] = it.value().size();
    }
    
    // 拓扑排序
    QList<QString> queue;
    
    // 找到所有入度为0的节点
    for (auto it = inDegree.begin(); it != inDegree.end(); ++it) {
        if (it.value() == 0) {
            queue.append(it.key());
        }
    }
    
    while (!queue.isEmpty()) {
        QString current = queue.takeFirst();
        executionOrder.append(current);
        
        // 更新依赖当前节点的其他节点的入度
        for (auto it = dependencies.begin(); it != dependencies.end(); ++it) {
            if (it.value().contains(current)) {
                inDegree[it.key()]--;
                if (inDegree[it.key()] == 0) {
                    queue.append(it.key());
                }
            }
        }
    }
    
    return executionOrder;
}

/**
 * @brief 生成代码文件头部
 * @return 头部代码字符串，包含注释和必要的包含语句
 */
QString CodeGenerator::generateHeader()
{
    QString header = R"(
/**
 * @file generated_code.cpp
 * @brief 自动生成的信号处理代码
 * @date %1
 * 
 * 此文件由Qt节点编辑器自动生成，包含完整的信号处理流程。
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <complex>
#include <string>

using namespace std;

// 信号类型定义
typedef vector<double> Signal;
typedef complex<double> ComplexSignal;

// 前向声明
)";

    return header.arg(QDateTime::currentDateTime().toString(Qt::ISODate));
}

/**
 * @brief 生成单个节点的代码
 * @param node 节点的JSON数据，包含类型、名称、参数等信息
 * @return 节点对应的代码字符串
 */
QString CodeGenerator::generateNodeCode(const QJsonObject &node)
{
    QString nodeType = node["type"].toString();
    QString nodeName = node["name"].toString();
    QString nodeId = node["id"].toString();
    
    QString code = generateVariableDeclaration(nodeType, nodeName);
    code += generateInitializationCode(node);
    code += "\n";
    
    return code;
}

/**
 * @brief 根据节点类型生成对应的变量声明
 * @param nodeType 节点类型
 * @param nodeName 节点名称
 * @return 变量声明代码
 */
QString CodeGenerator::generateVariableDeclaration(const QString &nodeType, const QString &nodeName)
{
    if (nodeType == "signal_source") {
        return QString("Signal %1;\n").arg(nodeName);
    } else if (nodeType == "filter") {
        return QString("Signal %1;\nSignal %1_filtered;\n").arg(nodeName);
    } else if (nodeType == "fft") {
        return QString("Signal %1;\nvector<ComplexSignal> %1_spectrum;\n").arg(nodeName);
    } else if (nodeType == "modulator") {
        return QString("Signal %1;\nSignal %1_modulated;\n").arg(nodeName);
    } else if (nodeType == "demodulator") {
        return QString("Signal %1;\nSignal %1_demodulated;\n").arg(nodeName);
    } else {
        return QString("Signal %1;\n").arg(nodeName);
    }
}

/**
 * @brief 根据节点类型生成初始化代码
 * @param node 节点的JSON数据
 * @return 初始化代码
 */
QString CodeGenerator::generateInitializationCode(const QJsonObject &node)
{
    QString nodeType = node["type"].toString();
    QString nodeName = node["name"].toString();
    
    if (nodeType == "signal_source") {
        return QString("// 初始化信号源 %1\n// TODO: 配置信号源参数\n").arg(nodeName);
    } else if (nodeType == "filter") {
        return QString("// 初始化滤波器 %1\n// TODO: 配置滤波器参数\n").arg(nodeName);
    } else if (nodeType == "fft") {
        return QString("// 初始化FFT变换 %1\n// TODO: 配置FFT参数\n").arg(nodeName);
    } else if (nodeType == "modulator") {
        return QString("// 初始化调制器 %1\n// TODO: 配置调制器参数\n").arg(nodeName);
    } else if (nodeType == "demodulator") {
        return QString("// 初始化解调器 %1\n// TODO: 配置解调器参数\n").arg(nodeName);
    } else {
        return QString("// 初始化节点 %1\n").arg(nodeName);
    }
}

/**
 * @brief 生成主函数代码
 * @param flowData 流程图的JSON数据
 * @return 主函数代码字符串
 */
QString CodeGenerator::generateMainFunction(const QJsonObject &flowData)
{
    // 分析依赖关系和执行顺序
    QMap<QString, QStringList> dependencies = analyzeDependencies(flowData);
    QStringList executionOrder = generateExecutionOrder(dependencies);
    
    QString mainFunction = R"(
/**
 * @brief 主处理函数，按依赖顺序执行所有节点
 */
int main()
{
    cout << "开始执行信号处理流程..." << endl;
    
)";

    // 按执行顺序生成处理代码
    for (const QString &nodeId : executionOrder) {
        // 找到对应的节点信息
        QJsonArray nodes = flowData["nodes"].toArray();
        QString nodeName;
        QString nodeType;
        
        for (const QJsonValue &nodeValue : nodes) {
            QJsonObject node = nodeValue.toObject();
            if (node["id"].toString() == nodeId) {
                nodeName = node["name"].toString();
                nodeType = node["type"].toString();
                break;
            }
        }
        
        if (nodeName.isEmpty()) continue;
        
        // 生成节点处理代码
        mainFunction += generateNodeProcessingCode(nodeName, nodeType, dependencies[nodeId]);
    }
    
    mainFunction += R"(
    
    cout << "信号处理流程执行完成。" << endl;
    return 0;
}
)";

    return mainFunction;
}

/**
 * @brief 生成节点处理代码
 * @param nodeName 节点名称
 * @param nodeType 节点类型
 * @param dependencies 依赖列表
 * @return 处理代码字符串
 */
QString CodeGenerator::generateNodeProcessingCode(const QString &nodeName, const QString &nodeType, const QStringList &dependencies)
{
    QString code = QString("    // 处理节点: %1\n").arg(nodeName);
    
    if (nodeType == "signal_source") {
        code += QString("    cout << \"生成信号源 %1 数据...\" << endl;\n").arg(nodeName);
        code += QString("    // TODO: 实现信号源生成逻辑\n");
        code += QString("    // %1.resize(1000); // 示例：设置信号长度\n").arg(nodeName);
    } else if (nodeType == "filter") {
        code += QString("    cout << \"应用滤波器 %1...\" << endl;\n").arg(nodeName);
        if (!dependencies.isEmpty()) {
            QString inputNode = dependencies.first(); // 简化处理，取第一个依赖
            code += QString("    // TODO: 从 %1 接收信号并进行滤波\n").arg(inputNode);
            code += QString("    // applyFilter(%1, %1_filtered);\n").arg(nodeName);
        }
    } else if (nodeType == "fft") {
        code += QString("    cout << \"执行FFT变换 %1...\" << endl;\n").arg(nodeName);
        if (!dependencies.isEmpty()) {
            QString inputNode = dependencies.first();
            code += QString("    // TODO: 从 %1 接收信号并进行FFT变换\n").arg(inputNode);
            code += QString("    // performFFT(%1, %1_spectrum);\n").arg(nodeName);
        }
    } else if (nodeType == "modulator") {
        code += QString("    cout << \"执行调制 %1...\" << endl;\n").arg(nodeName);
        if (!dependencies.isEmpty()) {
            QString inputNode = dependencies.first();
            code += QString("    // TODO: 从 %1 接收信号并进行调制\n").arg(inputNode);
            code += QString("    // performModulation(%1, %1_modulated);\n").arg(nodeName);
        }
    } else if (nodeType == "demodulator") {
        code += QString("    cout << \"执行解调 %1...\" << endl;\n").arg(nodeName);
        if (!dependencies.isEmpty()) {
            QString inputNode = dependencies.first();
            code += QString("    // TODO: 从 %1 接收信号并进行解调\n").arg(inputNode);
            code += QString("    // performDemodulation(%1, %1_demodulated);\n").arg(nodeName);
        }
    } else {
        code += QString("    cout << \"处理节点 %1...\" << endl;\n").arg(nodeName);
        code += QString("    // TODO: 实现节点 %1 的处理逻辑\n").arg(nodeName);
    }
    
    code += "\n";
    return code;
}

/**
 * @brief 生成代码文件尾部
 * @return 尾部代码字符串
 */
QString CodeGenerator::generateFooter()
{
    return R"(

// ==================== 辅助函数 ====================

/**
 * @brief 滤波器处理函数
 * @param input 输入信号
 * @param output 输出信号
 */
void applyFilter(const Signal& input, Signal& output) {
    // TODO: 实现滤波算法
    output = input; // 临时实现
}

/**
 * @brief FFT变换函数
 * @param input 输入信号
 * @param spectrum 输出频谱
 */
void performFFT(const Signal& input, vector<ComplexSignal>& spectrum) {
    // TODO: 实现FFT算法
    spectrum.clear();
}

/**
 * @brief 调制函数
 * @param input 输入信号
 * @param output 输出调制信号
 */
void performModulation(const Signal& input, Signal& output) {
    // TODO: 实现调制算法
    output = input; // 临时实现
}

/**
 * @brief 解调函数
 * @param input 输入信号
 * @param output 输出解调信号
 */
void performDemodulation(const Signal& input, Signal& output) {
    // TODO: 实现解调算法
    output = input; // 临时实现
}

/*
 * 生成完毕 - 此文件由Qt节点编辑器自动生成
 */
)";
}

/**
 * @brief 根据标准JSON格式生成Python代码
 * @param flowData 标准流程图的JSON数据
 * @return 生成的Python代码字符串
 */
QString CodeGenerator::generatePythonCode(const QJsonObject &flowData)
{
    QString code;
    
    // 文件头
    code += "#!/usr/bin/env python3\n";
    code += "# -*- coding: utf-8 -*-\n";
    code += QString("# 自动生成的节点流程代码\n");
    code += QString("# 生成时间: %1\n\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    
    code += "import json\n";
    code += "import numpy as np\n\n";
    
    // 节点类定义
    code += "class Node:\n";
    code += "    def __init__(self, node_id, node_type, name, position):\n";
    code += "        self.id = node_id\n";
    code += "        self.type = node_type\n";
    code += "        self.name = name\n";
    code += "        self.position = position\n";
    code += "        self.inputs = []\n";
    code += "        self.outputs = []\n\n";
    code += "    def process(self, data):\n";
    code += "        # 根据节点类型处理数据\n";
    code += "        return data\n\n";
    
    // 流程图类
    code += "class FlowGraph:\n";
    code += "    def __init__(self):\n";
    code += "        self.nodes = {}\n";
    code += "        self.connections = []\n\n";
    
    code += "    def add_node(self, node):\n";
    code += "        self.nodes[node.id] = node\n\n";
    
    code += "    def add_connection(self, from_id, to_id, from_port=0, to_port=0):\n";
    code += "        self.connections.append({\n";
    code += "            'from': from_id,\n";
    code += "            'to': to_id,\n";
    code += "            'fromPort': from_port,\n";
    code += "            'toPort': to_port\n";
    code += "        })\n\n";
    
    code += "    def execute(self):\n";
    code += "        # 按拓扑顺序执行节点\n";
    code += "        print('执行流程图...')\n";
    code += "        for conn in self.connections:\n";
    code += "            print(f\"  {conn['from']} -> {conn['to']}\")\n\n";
    
    // 主程序
    code += "# 创建流程图\n";
    code += "graph = FlowGraph()\n\n";
    
    // 添加节点
    code += "# 添加节点\n";
    QJsonArray nodes = flowData["nodes"].toArray();
    for (const QJsonValue &nodeValue : nodes) {
        QJsonObject node = nodeValue.toObject();
        QString nodeId = node["id"].toString();
        QString nodeType = node["type"].toString();
        QString nodeName = node["name"].toString();
        
        double x = 0, y = 0;
        if (node.contains("position")) {
            QJsonObject pos = node["position"].toObject();
            x = pos["x"].toDouble();
            y = pos["y"].toDouble();
        }
        
        code += QString("graph.add_node(Node('%1', '%2', '%3', {'x': %4, 'y': %5}))\n")
            .arg(nodeId).arg(nodeType).arg(nodeName).arg(x).arg(y);
    }
    
    code += "\n# 添加连接\n";
    QJsonArray connections = flowData["connections"].toArray();
    for (const QJsonValue &connValue : connections) {
        QJsonObject conn = connValue.toObject();
        QString fromId = conn["from"].toString();
        QString toId = conn["to"].toString();
        int fromPort = conn["fromPort"].toInt(0);
        int toPort = conn["toPort"].toInt(0);
        
        code += QString("graph.add_connection('%1', '%2', %3, %4)\n")
            .arg(fromId).arg(toId).arg(fromPort).arg(toPort);
    }
    
    code += "\n# 执行流程\n";
    code += "if __name__ == '__main__':\n";
    code += "    graph.execute()\n";
    
    return code;
}

/**
 * @brief 根据标准JSON格式生成配置文件
 * @param flowData 标准流程图的JSON数据
 * @return 配置文件内容（YAML格式）
 */
QString CodeGenerator::generateConfigFile(const QJsonObject &flowData)
{
    QString config;
    
    // YAML头
    config += "# 节点流程图配置文件\n";
    config += QString("# 生成时间: %1\n\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    
    // 元数据
    config += "metadata:\n";
    if (flowData.contains("metadata")) {
        QJsonObject metadata = flowData["metadata"].toObject();
        config += QString("  title: \"%1\"\n").arg(metadata["title"].toString());
        config += QString("  version: \"%1\"\n").arg(metadata["version"].toString());
        config += QString("  created: \"%1\"\n").arg(metadata["created"].toString());
    }
    config += "\n";
    
    // 节点
    config += "nodes:\n";
    QJsonArray nodes = flowData["nodes"].toArray();
    for (const QJsonValue &nodeValue : nodes) {
        QJsonObject node = nodeValue.toObject();
        config += QString("  - id: %1\n").arg(node["id"].toString());
        config += QString("    type: %1\n").arg(node["type"].toString());
        config += QString("    name: \"%1\"\n").arg(node["name"].toString());
        
        if (node.contains("position")) {
            QJsonObject pos = node["position"].toObject();
            config += "    position:\n";
            config += QString("      x: %1\n").arg(pos["x"].toDouble());
            config += QString("      y: %1\n").arg(pos["y"].toDouble());
        }
        
        if (node.contains("inputPortCount")) {
            config += QString("    inputPorts: %1\n").arg(node["inputPortCount"].toInt());
        }
        if (node.contains("outputPortCount")) {
            config += QString("    outputPorts: %1\n").arg(node["outputPortCount"].toInt());
        }
        config += "\n";
    }
    
    // 连接
    config += "connections:\n";
    QJsonArray connections = flowData["connections"].toArray();
    for (const QJsonValue &connValue : connections) {
        QJsonObject conn = connValue.toObject();
        config += QString("  - from: %1\n").arg(conn["from"].toString());
        config += QString("    to: %1\n").arg(conn["to"].toString());
        if (conn.contains("fromPort")) {
            config += QString("    fromPort: %1\n").arg(conn["fromPort"].toInt());
        }
        if (conn.contains("toPort")) {
            config += QString("    toPort: %1\n").arg(conn["toPort"].toInt());
        }
        config += "\n";
    }
    
    return config;
}
