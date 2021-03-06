/*********************************************************************/
/* Copyright (c) 2013-2018, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
/*                          Daniel.Nachbaur@epfl.ch                  */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE  IS  PROVIDED  BY  THE  ECOLE  POLYTECHNIQUE    */
/*    FEDERALE DE LAUSANNE  ''AS IS''  AND ANY EXPRESS OR IMPLIED    */
/*    WARRANTIES, INCLUDING, BUT  NOT  LIMITED  TO,  THE  IMPLIED    */
/*    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  A PARTICULAR    */
/*    PURPOSE  ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  ECOLE    */
/*    POLYTECHNIQUE  FEDERALE  DE  LAUSANNE  OR  CONTRIBUTORS  BE    */
/*    LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,    */
/*    EXEMPLARY,  OR  CONSEQUENTIAL  DAMAGES  (INCLUDING, BUT NOT    */
/*    LIMITED TO,  PROCUREMENT  OF  SUBSTITUTE GOODS OR SERVICES;    */
/*    LOSS OF USE, DATA, OR  PROFITS;  OR  BUSINESS INTERRUPTION)    */
/*    HOWEVER CAUSED AND  ON ANY THEORY OF LIABILITY,  WHETHER IN    */
/*    CONTRACT, STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE    */
/*    OR OTHERWISE) ARISING  IN ANY WAY  OUT OF  THE USE OF  THIS    */
/*    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.   */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of Ecole polytechnique federale de Lausanne.          */
/*********************************************************************/

#ifndef DEFLECT_SERVER_SERVER_H
#define DEFLECT_SERVER_SERVER_H

#include <deflect/SizeHints.h>
#include <deflect/api.h>
#include <deflect/server/types.h>

#include <QObject>

namespace deflect
{
namespace server
{
/**
 * Listen to incoming connections from multiple Stream clients.
 *
 * Both mono and frame-sequential stereo 3D streams are supported.
 *
 * The server integrates a flow-control mechanism to ensure that new frames are
 * dispatched only as fast as the application is capable of processing them.
 */
class DEFLECT_API Server : public QObject
{
    Q_OBJECT

public:
    /** The default port number used for Stream connections. */
    static const int defaultPortNumber;

    /**
     * Create a new server listening for Stream connections.
     *
     * @param port The port to listen on. Must be available.
     * @throw std::runtime_error if the server could not be started.
     */
    explicit Server(int port = defaultPortNumber);

    /** Stop the server and close all open pixel stream connections. */
    ~Server();

    /** @return the port on which the server is running. */
    quint16 getPort() const;

public slots:
    /**
     * Request the dispatching of the next frame for a given pixel stream.
     *
     * A receivedFrame() signal will subsequently be emitted for each of the
     * view(s) (mono or stereo) for which a frame is or becomes available.
     *
     * To ensure that the two eye channels remain synchronized, stereo
     * left/right frames are dispatched together only when both are available.
     *
     * @param uri Identifier for the stream
     */
    void requestFrame(QString uri);

    /**
     * Close a pixel stream, disconnecting the remote client.
     *
     * @param uri Identifier for the stream
     */
    void closePixelStream(QString uri);

signals:
    /**
     * Notify that a pixel stream has been opened.
     *
     * @param uri Identifier for the stream
     */
    void pixelStreamOpened(QString uri);

    /**
     * Notify that a stream has encountered an exception.
     *
     * Used for error reporting.
     *
     * @param uri Identifier for the stream
     * @param what The error message
     */
    void pixelStreamException(QString uri, QString what);

    /**
     * Notify that a pixel stream has been closed.
     *
     * @param uri Identifier for the stream
     */
    void pixelStreamClosed(QString uri);

    /**
     * Emitted when a full frame has been received from a pixel stream.
     *
     * This signal is only emitted after the application signals that it is
     * ready to handle a new frame by calling requestFrame().
     *
     * @param frame The latest frame that was received for a stream.
     */
    void receivedFrame(deflect::server::FramePtr frame);

    /**
     * Emitted when a remote client wants to register for receiving events.
     *
     * @param uri Identifier for the stream
     * @param exclusive true if the receiver should receive events exclusively
     * @param receiver the event receiver instance
     * @param success the promise that must receive the success of the operation
     */
    void registerToEvents(QString uri, bool exclusive,
                          deflect::server::EventReceiver* receiver,
                          deflect::server::BoolPromisePtr success);

    /**
     * Emitted when a remote client sends size hints for displaying the stream.
     *
     * @param uri Identifier for the stream
     * @param hints The size hints to apply
     */
    void receivedSizeHints(QString uri, deflect::SizeHints hints);

    /**
     * Emitted when a remote client sends generic data.
     *
     * @param uri Identifier for the stream
     * @param data A streamer-specific message
     */
    void receivedData(QString uri, QByteArray data);

private:
    class Impl;
    std::unique_ptr<Impl> _impl;

signals:
    /** @internal */
    void _closePixelStream(QString uri);
};
}
}

#endif
