/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
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
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include "NetworkListener.h"

#include "NetworkListenerWorker.h"
#include "PixelStreamDispatcher.h"

#include "CommandHandler.h"

#include <QThread>
#include <stdexcept>

namespace deflect
{

const int NetworkListener::defaultPortNumber_ = 1701;

NetworkListener::NetworkListener( int port)
    : pixelStreamDispatcher_(new PixelStreamDispatcher())
    , commandHandler_(new CommandHandler())
{
    if( !listen(QHostAddress::Any, port) )
    {
        const QString err = QString("could not listen on port: %1").arg(port);
        throw std::runtime_error(err.toStdString());
    }
}

NetworkListener::~NetworkListener()
{
    delete pixelStreamDispatcher_;
    delete commandHandler_;
}

CommandHandler& NetworkListener::getCommandHandler()
{
    return *commandHandler_;
}

PixelStreamDispatcher& NetworkListener::getPixelStreamDispatcher()
{
    return *pixelStreamDispatcher_;
}

void NetworkListener::incomingConnection(int socketHandle)
{
    QThread * workerThread = new QThread();
    NetworkListenerWorker * worker = new NetworkListenerWorker(socketHandle);

    worker->moveToThread(workerThread);

    connect(workerThread, SIGNAL(started()), worker, SLOT(initialize()));
    connect(worker, SIGNAL(finished()), workerThread, SLOT(quit()));

    // Make sure the thread will be deleted
    connect(workerThread, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));

    // public signals/slots, forwarding from/to worker
    connect(worker,
            SIGNAL(registerToEvents(QString, bool, deflect::EventReceiver*)),
            this,
            SIGNAL(registerToEvents(QString, bool, deflect::EventReceiver*)));
    connect(this, SIGNAL(pixelStreamerClosed(QString)),
            worker, SLOT(pixelStreamerClosed(QString)));
    connect(this, SIGNAL(eventRegistrationReply(QString, bool)),
            worker, SLOT(eventRegistrationReply(QString, bool)));

    // Commands
    connect(worker, SIGNAL(receivedCommand(QString,QString)),
            commandHandler_, SLOT(process(QString,QString)));

    // PixelStreamDispatcher
    connect(worker, SIGNAL(receivedAddPixelStreamSource(QString,size_t)),
            pixelStreamDispatcher_, SLOT(addSource(QString,size_t)));
    connect(worker,
            SIGNAL(receivedPixelStreamSegement(QString,size_t,
                                               PixelStreamSegment)),
            pixelStreamDispatcher_,
            SLOT(processSegment(QString,size_t,PixelStreamSegment)));
    connect(worker,
            SIGNAL(receivedPixelStreamFinishFrame(QString,size_t)),
            pixelStreamDispatcher_, SLOT(processFrameFinished(QString,size_t)));
    connect(worker, SIGNAL(receivedRemovePixelStreamSource(QString,size_t)),
            pixelStreamDispatcher_, SLOT(removeSource(QString,size_t)));

    workerThread->start();
}

void NetworkListener::onPixelStreamerClosed( QString uri )
{
    emit pixelStreamerClosed( uri );
}

void NetworkListener::onEventRegistrationReply( QString uri, bool success )
{
    emit eventRegistrationReply( uri, success );
}

}