#pragma once
#include <QObject>
#include <QTcpSocket>
#include <aseba/flatbuffers/fb_message_ptr.h>
#include <QUrl>
#include "request.h"
#include "thymionode.h"

namespace mobsya {
class ThymioDeviceManagerClientEndpoint : public QObject,
                                          public std::enable_shared_from_this<ThymioDeviceManagerClientEndpoint> {
    Q_OBJECT

public:
    ThymioDeviceManagerClientEndpoint(QTcpSocket* socket, QObject* parent = nullptr);
    ~ThymioDeviceManagerClientEndpoint();

public:
    std::shared_ptr<ThymioNode> node(const QUuid& id) const;

    QHostAddress peerAddress() const;
    QUrl websocketConnectionUrl() const;
    void setWebSocketMatchingPort(quint16 port);

    bool isLocalhostPeer() const;
    Request requestDeviceManagerShutdown();


    Request renameNode(const ThymioNode& node, const QString& newName);
    Request setNodeExecutionState(const ThymioNode& node, fb::VMExecutionStateCommand cmd);
    BreakpointsRequest setNodeBreakPoints(const ThymioNode& node, const QVector<unsigned>& breakpoints);
    Request lock(const ThymioNode& node);
    Request unlock(const ThymioNode& node);
    CompilationRequest send_code(const ThymioNode& node, const QByteArray& code, fb::ProgrammingLanguage language,
                                 fb::CompilationOptions opts);
    Request send_aesl(const ThymioGroup& group, const QByteArray& code);
    Request set_watch_flags(const QUuid& node, int flags);
    AsebaVMDescriptionRequest fetchAsebaVMDescription(const ThymioNode& node);
    Request setNodeVariables(const ThymioNode& node, const ThymioNode::VariableMap& vars);
    Request setGroupVariables(const ThymioGroup& group, const ThymioNode::VariableMap& vars);
    Request setNodeEventsTable(const QUuid& id, const QVector<EventDescription>& events);
    Request emitNodeEvents(const ThymioNode& node, const ThymioNode::EventMap& vars);
    Request setScratchPad(const QUuid& id, const QByteArray& data, fb::ProgrammingLanguage language);

private Q_SLOTS:
    void onReadyRead();
    void onConnected();
    void cancelAllRequests();
    void write(const flatbuffers::DetachedBuffer& buffer);
    void write(const tagged_detached_flatbuffer& buffer);

Q_SIGNALS:
    void nodeAdded(std::shared_ptr<ThymioNode>);
    void nodeRemoved(std::shared_ptr<ThymioNode>);
    void nodeModified(std::shared_ptr<ThymioNode>);

    void onMessage(const fb_message_ptr& msg) const;
    void disconnected();

private:
    template <typename T>
    T prepare_request() {
        T r = T::make_request();
        m_pending_requests.insert(r.id(), r.get_ptr());
        return r;
    }

    void onNodesChanged(const fb::NodesChangedT& nc_msg);
    void handleIncommingMessage(const fb_message_ptr& msg);
    detail::RequestDataBase::shared_ptr get_request(detail::RequestDataBase::request_id);
    std::vector<std::shared_ptr<ThymioNode>> nodes(const QUuid& node_or_group_id) const;
    std::shared_ptr<ThymioGroup> group_from_group_id(const QUuid& id);
    std::shared_ptr<ThymioGroup> group_from_id(const QUuid& id);

    Request setVariables(const QUuid& id, const ThymioNode::VariableMap& vars);


    static flatbuffers::Offset<fb::NodeId> serialize_uuid(flatbuffers::FlatBufferBuilder& fb, const QUuid& uuid);
    QMap<QUuid, std::shared_ptr<ThymioNode>> m_nodes;
    QTcpSocket* m_socket;
    quint32 m_message_size;
    quint16 m_ws_port = 0;
    bool m_islocalhostPeer = false;
    QMap<detail::RequestDataBase::request_id, detail::RequestDataBase::shared_ptr> m_pending_requests;
};

}  // namespace mobsya
