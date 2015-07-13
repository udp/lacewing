
/* vim: set et ts=4 sw=4 ft=actionscript:
 *
 * Copyright (C) 2011 James McLaughlin.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

package lacewing
{
    import flash.events.Event;
    import flash.events.IOErrorEvent;
    import flash.net.Socket;
    import flash.utils.ByteArray;
    import flash.utils.Endian;
    
    public class Client
    {
        /* Handlers to override */
        
        public function onConnect(welcomeMessage:String):void
        {
        }
        
        public function onConnectDenied(reason:String):void
        {
        }
        
        public function onDisconnect():void
        {
        }
        
        public function onError(error:Error):void
        {
        }
        
        public function onNameSet():void
        {
        }
        
        public function onNameChanged(oldName:String):void
        {
        }
        
        public function onNameDenied(name:String, reason:String):void
        {
        }
        
        public function onServerMessage(subchannel:int, message:flash.utils.ByteArray, variant:int):void
        {
        }
        
        public function onChannelMessage(channel:Channel, peer:Peer, subchannel:int, message:flash.utils.ByteArray, variant:int):void
        {
        }
        
        public function onPeerMessage(channel:Channel, peer:Peer, subchannel:int, message:flash.utils.ByteArray, variant:int):void
        {
        }
        
        public function onServerChannelMessage(channel:Channel, subchannel:int, message:flash.utils.ByteArray, variant:int):void
        {
        }
        
        public function onJoinChannel(channel:Channel):void
        {
        }
        
        public function onJoinChannelDenied(channelName:String, denyReason:String):void
        {
        }
        
        public function onLeaveChannel(channel:Channel):void
        {
        }
        
        public function onLeaveChannelDenied(channel:Channel, reason:String):void
        {
        }
        
        public function onPeerConnect(channel:Channel, peer:Peer):void
        {
        }
        
        public function onPeerDisconnect(channel:Channel, peer:Peer):void
        {
        }
        
        public function onPeerChangeName(channel:Channel, peer:Peer, oldName:String):void
        {
        }
        
        public function onChannelListReceived():void
        {
        }    
        
        
        /* Public interface */
        
        public static const charset:String = "iso-8859-1";
        
        public function Client()
        {
            socket           = new flash.net.Socket;
            message          = new flash.utils.ByteArray;
            received         = new flash.utils.ByteArray;
            receivedThisTime = new flash.utils.ByteArray;
            
            socket.endian = message.endian = received.endian
                = receivedThisTime.endian = flash.utils.Endian.LITTLE_ENDIAN;

            socket.addEventListener(Event.CONNECT, onSocketConnect);
            socket.addEventListener(Event.CLOSE, onSocketDisconnect);
            socket.addEventListener(IOErrorEvent.IO_ERROR, onSocketError);
            socket.addEventListener(flash.events.ProgressEvent.SOCKET_DATA, onSocketReceive);
            
            channels    = new Array();
            channelList = new Array();
            
            port = -1;
            
            reset();
        }
        
        public function getName():String
        {    return name;
        }
        
        public function getID():int
        {    return id;
        }
        
        public function getChannelList():Array
        {   return channelList;
        }
        
        public function getChannels():Array
        {   return channels;
        }
        
        public function getPort():int
        {   return this.port;
        }
        
        public function isConnected():Boolean
        {    return socket.connected;
        }
        
        public function connect(host:String, port:int = 6121):void
        {
            disconnect();
            
            if (port == 0)
                port = 6121;
            
            var hostElements:Array = host.split(":");
            
            if (hostElements.length > 1)
            {   host = hostElements[0];
                port = parseInt(hostElements[1], 10);
            }
            
            this.port = port;
            
            try
            {    socket.connect(host, port);
            }
            catch(e:Error)
            {    onError(e);
                this.port = -1;
            }
        }
        
        public function disconnect():void
        {
            if (socket.connected)
                socket.close();
            
            reset();
        }
            
        public function requestChannelList():void
        {   try
            {   addHeader (0, 0);       /* Request */
                message.writeByte(4);    /* ChannelList */
                
                send ();
            }
            catch(e:Error)
            {    onError(e);
            }
        }
        
        public function setName(newName:String):void
        {   try
            {   addHeader        (0, 0);  /* Request */
                message.writeByte   (1);  /* SetName */
                addString     (newName);

                send ();
            }
            catch(e:Error)
            {    onError(e);
            }
        }
        
        public function joinChannel(channelName:String, hidden:Boolean = false, autoClose:Boolean = false):void
        {
            try
            {   if(!getName().length)
                    throw "Set a name first";    
                
                addHeader           (0, 0); /* Request */
                message.writeByte   (2);    /* JoinChannel */
                message.writeByte   ((hidden ? 1 : 0) | (autoClose ? 2 : 0));
                addString           (channelName);
                
                send ();
            }
            catch(e:Error)
            {    onError(e);
            }
        }
        
        public function leaveChannel(channel:Channel):void
        {   try
            {   addHeader           (0, 0); /* Request */
                message.writeByte   (3);    /* LeaveChannel */
                message.writeShort  (channel.id);
                
                send ();
            }
            catch(e:Error)
            {    onError(e);
            }
        }
        
        public function sendServer(subchannel:int, message:flash.utils.ByteArray, variant:int = 0):void
        {   try
            {   addHeader (1, variant); /* BinaryServerMessage */
                
                this.message.writeByte   (subchannel);
                this.message.writeBytes  (message);
                
                send ();
            }
            catch(e:Error)
            {    onError(e);
            }
        }
        
        public function sendChannel(channel:Channel, subchannel:int, message:flash.utils.ByteArray, variant:int = 0):void
        {   try
            {    addHeader (2, variant); /* BinaryChannelMessage */
                
                this.message.writeByte   (subchannel);
                this.message.writeShort  (channel.id);
                this.message.writeBytes  (message);
                
                send ();
            }
            catch(e:Error)
            {    onError(e);
            }
        }
        
        public function sendPeer(channel:Channel, peer:Peer, subchannel:int, message:flash.utils.ByteArray, variant:int = 0):void
        {   try
            {    addHeader (3, variant); /* BinaryPeerMessage */
                
                this.message.writeByte   (subchannel);
                this.message.writeShort  (channel.id);
                this.message.writeShort  (peer.id);
                this.message.writeBytes  (message);
                
                send ();
            }
            catch(e:Error)
            {    onError(e);
            }
        }
        
        
        /* Private */
        
        private var socket:Socket;
        private var port:int;
        private var message:flash.utils.ByteArray;
        
        private var id:int;
        private var name:String;
        private var channels:Array;
        
        private var received:flash.utils.ByteArray;
        private var receivedThisTime:flash.utils.ByteArray;
        private var readerState:int;
        private var incomingType:int;
        private var sizeBytesLeft:int;
        private var incomingSize:int;
        
        private var channelList:Array;
        
        private function reset():void
        {
            name = "";
            id   = -1;
            port = -1;
            
            channels.length    = 0;
            channelList.length = 0;
            
            readerState = 0;
        }
        
        private function onSocketConnect(event:Event):void
        {
            socket.writeByte (0); /* Opening byte */
            
            addHeader(0, 0);      /* Request */
            message.writeByte(0); /* Connect */
            
            addString("revision 3");
            send();
        }
        
        private function onSocketDisconnect(event:Event):void
        {
            onDisconnect();
            reset();
        }
        
        private function onSocketReceive(event:Event):void
        {
            receivedThisTime.length   = 0;
            socket.readBytes(receivedThisTime);
            receivedThisTime.position = 0;

            process(receivedThisTime);
            
            receivedThisTime.length = 0;
        }
        
        private function onSocketError(event:IOErrorEvent):void
        {
            onError (new Error (event.toString ()));
        }
        
        private function process(bytes:flash.utils.ByteArray):void
        {

            while(bytes.bytesAvailable)
            {
                var byte:int = bytes.readByte();
                
                switch(readerState)
                {
                    case 0: /* Haven't yet got type */
                        
                        incomingType = byte;
                        readerState = 1;
                        sizeBytesLeft = 0;
                        
                        break;
                    
                    case 1: /* Have type, but no size */
                    {
                        if(sizeBytesLeft > 0)
                        {
                            received.writeByte(byte);
                            
                            if((-- sizeBytesLeft) == 0)
                            {
                                received.position = 0;
                                
                                if(received.length == 2)
                                {
                                    incomingSize = received.readUnsignedShort();
                                    break;
                                }
                                
                                if(received.length == 4)
                                {
                                    incomingSize = received.readUnsignedInt();
                                    break;
                                }
                                
                                received.length   = 0;
                                received.position = 0;
                                
                                readerState     = 3;
                                
                                break;
                            }
                            
                            break;
                        }
                        
                        if(byte == 254)
                        {
                            /* 16 bit message size to follow */
                            
                            sizeBytesLeft = 2;
                            break;
                        }
                        
                        if(byte == 255)
                        {
                            /* 32 bit message size to follow */
                            
                            sizeBytesLeft = 4;
                            break;
                        }
                        
                        /* 8 bit message size */
                        
                        incomingSize = byte;
                        readerState = 3;
                        
                        break;
                    }
                };
                
                if(readerState == 3)
                    break;
            }
            
            if(readerState == 3) /* Receiving message body */
            {
                if(received.bytesAvailable == 0 && bytes.bytesAvailable >= incomingSize)
                {
                    /* Often the case - the message isn't fragmented, no need to copy anything. */

                    handleMessage(incomingType, bytes, incomingSize);
                    readerState = 0;
                    
                    if(bytes.bytesAvailable > 0)
                        process(bytes);
                    
                    return;
                }
                
                var thisMessageBytes:int = incomingSize - received.length;
                
                if(bytes.bytesAvailable < thisMessageBytes)
                    thisMessageBytes = bytes.bytesAvailable;
                
                received.writeBytes(bytes, bytes.position, thisMessageBytes);
                bytes.position += thisMessageBytes;
                
                if(received.length == incomingSize)
                {
                    received.position = 0;
                    handleMessage(incomingType, received, incomingSize);
                    received.length = 0;
                    
                    readerState = 0;
                    
                    if(bytes.bytesAvailable > 0)
                        process(bytes);
                }
                
                return;
            }   
        }
        
        private function getRemainingString(byteArray:flash.utils.ByteArray, end:int):String
        {
            return byteArray.readMultiByte(end - byteArray.position, charset);
        }
        
        private function getSizedString(byteArray:flash.utils.ByteArray):String
        {
            return byteArray.readMultiByte(byteArray.readUnsignedByte(), charset);
        }
        
        private function getChannel(byteArray:flash.utils.ByteArray):Channel
        {
            var id:int = byteArray.readUnsignedShort();   
            
            for(var i:int = 0; i < channels.length; ++ i)
                if(channels[i].id == id)
                    return channels[i];
            
            trace("Can't find channel " + id);
            
            return null;
        }
        
        private function getPeer(channel:Channel, byteArray:flash.utils.ByteArray):Peer
        {
            var id:int = byteArray.readUnsignedShort();   
            
            for(var i:int = 0; i < channel.peers.length; ++ i)
                if(channel.peers[i].id == id)
                    return channel.peers[i];
            
            return null; 
        }

        private function readBinary(bytes:ByteArray, size:int):ByteArray
        {
            var binaryMessage:ByteArray = new ByteArray();
            binaryMessage.endian = flash.utils.Endian.LITTLE_ENDIAN;
            bytes.readBytes(binaryMessage, 0, size);
            return binaryMessage;
        }
        
        private function handleMessage(type:int, bytes:ByteArray, size:int):void
        {
            var messageTypeID:int = (type & 240) >>> 4;
            var variant:int = type & 15;
          
            var readEnd:int = bytes.position + size;
            
            var channel:Channel;
            var subchannel:int;
            var peerID:int;
            var peer:Peer;
            var flags:int;
            var newName:String;
            var listing:ChannelListing;
            var oldName:String;
            var channelName:String;
            var peerName:String;
            var binaryMessage:ByteArray;
            var i:int;
            
            switch(messageTypeID)
            {
                case 0: /* Response */
                {
                    var responseType:int  = bytes.readByte();
                    var succeeded:Boolean = bytes.readByte() != 0;

                    switch(responseType)
                    {
                        case 0: /* Connect */
                        {
                            if(succeeded)
                            {   id = bytes.readUnsignedShort();
                                onConnect(getRemainingString(bytes, readEnd));
                            }
                            else
                            {   onConnectDenied(getRemainingString(bytes, readEnd));
                            }
                            
                            break;
                        }
                            
                        case 1: /* SetName */
                        {
                            newName = getSizedString(bytes);

                            if(succeeded)
                            {
                                if(name == "")
                                {
                                    name = newName;
                                    onNameSet();
                                }
                                else
                                {
                                    oldName = name;
                                    name = newName;
                                    
                                    onNameChanged(oldName);   
                                }
                            }
                            else
                            {   onNameDenied(newName, getRemainingString(bytes, readEnd));
                            }
                            
                            break;
                        }
                            
                        case 2: /* JoinChannel */
                        {
                            flags = bytes.readUnsignedByte();
                            channelName = getSizedString(bytes);
                            
                            if(succeeded)
                            {
                                channel = new Channel(this);
                                
                                channel.id              = bytes.readUnsignedShort();
                                channel.name            = channelName;
                                channel.peers           = new Array();
                                channel.isChannelMaster = (flags & 1) != 0;
                                
                                for(;;)
                                {
                                    try
                                    {
                                        peer = new Peer(this);
                                    
                                        peer.id              = bytes.readUnsignedShort();
                                        peer.isChannelMaster = (bytes.readByte() & 1) != 0;
                                        peer.name            = getSizedString(bytes);
                                        peer.channel         = channel;
                                        
                                        peer.channel = channel;
                                    }
                                    catch (e:Error)
                                    {
                                        break;
                                    }
                                    
                                    channel.peers.push(peer);
                                }
                                
                                channels.push(channel);
                                
                                onJoinChannel(channel);
                            }
                            else
                            {   onJoinChannelDenied(channelName, getRemainingString(bytes, readEnd));
                            }
                            
                            break;
                        }
                            
                        case 3: /* LeaveChannel */
                        {
                            channel = getChannel(bytes);
                            
                            if(succeeded)
                            {
                                channels.splice (channels.indexOf (channel), 1);
                                
                                onLeaveChannel(channel);
                            }
                            else
                            {   onLeaveChannelDenied(channel, getRemainingString(bytes, readEnd));
                            }
                            
                            break;
                        }
                            
                        case 4: /* ChannelList */
                        {
                            channelList.length = 0;
                            
                            for(;;)
                            {
                                listing = new ChannelListing();
                                
                                try
                                {   listing.peerCount = bytes.readUnsignedShort();
                                    listing.name = getSizedString(bytes);
                                }
                                catch(e:Error)
                                {   break;
                                }
                                
                                channelList.push(listing);
                            }
                            
                            onChannelListReceived();
                            
                            break;
                        }
                            
                        default:
                            break;
                    }
                    
                    break;
                }
                    
                case 1: /* BinaryServerMessage */
                {
                    subchannel = bytes.readUnsignedByte();

                    binaryMessage = readBinary(bytes, readEnd - bytes.position);
                    
                    onServerMessage(subchannel, binaryMessage, variant);
                    
                    break;
                }
                    
                case 2: /* BinaryChannelMessage */
                {
                    subchannel = bytes.readUnsignedByte();        
                    channel    = getChannel(bytes);
                    peer       = getPeer(channel, bytes);

                    binaryMessage = readBinary(bytes, readEnd - bytes.position);
                    
                    onChannelMessage(channel, peer, subchannel, binaryMessage, 
                                     variant);
                    
                    break;
                }
                    
                case 3: /* BinaryPeerMessage */
                {
                    subchannel = bytes.readUnsignedByte();        
                    channel    = getChannel(bytes);
                    peer       = getPeer(channel, bytes);

                    binaryMessage = readBinary(bytes, readEnd - bytes.position);
                    
                    onPeerMessage(channel, peer, subchannel, binaryMessage, 
                                  variant);
                    
                    break;
                }
                    
                case 4: /* BinaryServerChannelMessage */
                {
                    subchannel = bytes.readUnsignedByte();        
                    channel    = getChannel(bytes);

                    binaryMessage = readBinary(bytes, readEnd - bytes.position);
                    
                    onServerChannelMessage(channel, subchannel, binaryMessage, 
                                           variant);
                    
                    break;
                }
                
                case 5: /* ObjectServerMessage */
                case 6: /* ObjectChannelMessage */
                case 7: /* ObjectPeerMessage */
                case 8: /* ObjectServerChannelMessage */
                    
                    break;
                
                case 9: /* Peer */
                {
                    channel = getChannel(bytes);
                    peerID  = bytes.readUnsignedShort();
                    
                    for(i = 0; i < channel.peers.length; ++ i)
                    {
                        if(channel.peers[i].id == peerID)
                        {
                            peer = channel.peers[i];
                            break;
                        }
                    }
                    
                    if(bytes.position >= readEnd)
                    {
                        /* No flags/name - the peer must have left the channel */
                        
                        channel.peers.splice (channel.peers.indexOf (peer), 1);
                        onPeerDisconnect(channel, peer);
                        
                        break;
                    }
                    
                    flags = bytes.readUnsignedByte();
                    peerName = getRemainingString(bytes, readEnd);
                    
                    if(!peer)
                    {
                        /* New peer */
                        
                        peer = new Peer(this);
                        
                        peer.id              = peerID;
                        peer.name            = peerName;
                        peer.channel         = channel;
                        peer.isChannelMaster = (flags & 1) != 0; 
                        
                        channel.peers.push(peer);
                        
                        onPeerConnect(channel, peer);
                        
                        break;
                    }
                    
                    /* Existing peer */
                    
                    if(peerName != peer.name)
                    {
                        /* Peer is changing their name */
                        
                        oldName = peer.name;
                        peer.name = peerName;
                        
                        onPeerChangeName(channel, peer, oldName);
                    }
                    
                    break;
                }
                
                case 10: /* UDPWelcome */
                    break;
                
                case 11: /* Ping */
                    
                    addHeader(9, 0); /* Pong */
                    send ();
                    
                    break;
                    
                default:
                    break;
            };

            bytes.position = readEnd;
        }
        
        private function addHeader(type:int, variant:int):void
        {
            message.writeInt ((type << 4) | variant);
            message.writeInt (0);
        }
        
        private function addString(string:String):void
        {
            message.writeMultiByte(string, charset);   
        }
        
        private function send():void
        {
            message.position = 0;
            
            var type:int = message.readInt();
            var messageSize:int = message.length - 8;
            
            var headerSize:int;
            
            if(messageSize < 254)
            {
                message[6] = type;
                message[7] = messageSize;
                
                headerSize = 2;
            }
            else if(messageSize < 0xFFFF)
            {
                message[4] = type;
                message[5] = 254;
                
                message.position = 6;
                message.writeShort(messageSize);
            
                headerSize = 4;
            }
            else if(messageSize < 0xFFFFFFFF)
            {
                message[2] = type;
                message[3] = 255;
                
                message.position = 4;
                message.writeUnsignedInt(messageSize);

                headerSize = 6;
            }
            
            message.position = 0;
            
            socket.writeBytes(message, 8 - headerSize, messageSize + headerSize);
            socket.flush();
            
            message.length   = 0;
            message.position = 0;
        }
        
        private function byteArrayDebug(byteArray:flash.utils.ByteArray, offset:int = 0):String
        {
            var s:String = "";
            var p:int = byteArray.position;
            byteArray.position = offset;
            
            while(byteArray.bytesAvailable > 0)
            {
                var b:int = byteArray.readByte();
                s += b.toString(10) + " ";
                
                if(b != 0)
                    s += String.fromCharCode(b) + " ";
            }
            
            byteArray.position = p;
            return s;
        }
        
    }
}
