package Extensions
{
	import Services.*;
	
	import flash.utils.ByteArray;
	
	import lacewing.Channel;
	import lacewing.Client;
	import lacewing.Peer;
    import lacewing.ChannelListing;

	public class CRunLacewingGlobalData extends lacewing.Client
	{
		public var StackCursor:int;
		public var DenyReason:String;
		public var WelcomeMessage:String;
		public var PeerPreviousName:String;
		public var PreviousName:String;	
		public var ReceivedBegin:int;
		public var LastError:String;
		public var SelectedChannel:lacewing.Channel;
		public var Received:ByteArray;
		public var Stack:ByteArray;
		public var IncomingSubchannel:int;
		public var AutoClearStack:Boolean;
		public var Objects:Array;
		public var loopName:String;
		public var CurrentChannelListing:ChannelListing;
				
        public function generate(code:int):void
        {   for(var i:int = 0; i < Objects.length; ++ i)    
                (Objects[i] as CRunLacewing).ho.generateEvent(code, 0);
        }
        
        public override function onConnect(welcomeMessage:String):void
        {
            WelcomeMessage = welcomeMessage;
            
            generate(1);
        }
        
        public override function onDisconnect():void
        {   generate(3);
            reset();
        }
        
        public override function onNameSet():void
        {   generate(6);
        }
        
        public override function onNameChanged(oldName:String):void
        {
            PreviousName = oldName;
            generate(53);
        }
        
        public override function onNameDenied(name:String, reason:String):void
        {
            DenyReason = reason;
            generate(7);
        }
        
        public override function onPeerChangeName(channel:Channel, peer:Peer, oldName:String):void
        {
            PeerPreviousName = oldName;
            generate(45);
        }
        
        public override function onError(error:Error):void
        {
            LastError = error.toString();
            generate(0);
        }
        
        public override function onJoinChannel(channel:Channel):void
        {
            SelectedChannel = channel;
            generate(4);
        }
        
        public override function onLeaveChannel(channel:Channel):void
        {
            SelectedChannel = channel;
            generate(43);
            SelectedChannel = null;
        }
        
        public override function onLeaveChannelDenied(channel:Channel, reason:String):void
        {
            SelectedChannel = channel;
            DenyReason = reason;
            generate(44);
            DenyReason = "";
        }
        
        public override function onJoinChannelDenied(channel:String, reason:String):void
        {
            DenyReason = reason;
            generate(5);
        }
        
        public override function onPeerConnect(channel:Channel, peer:Peer):void
        {
            SelectedChannel = channel;
            SelectedChannel.SelectedPeer = peer;
            
            generate(10);
        }
        
        public override function onPeerDisconnect(channel:Channel, peer:Peer):void
        {
            SelectedChannel = channel;
            SelectedChannel.SelectedPeer = peer;
            
            generate(11);
            
            SelectedChannel.SelectedPeer = null;
        }
        
        public override function onChannelListReceived():void
        {   generate(26);   
        }
        
        public override function onServerMessage(subchannel:int, message:ByteArray, variant:int):void
        {
            if (message == null) {
                return;
            }

            IncomingSubchannel = subchannel;
            Received = message;
            ReceivedBegin = message.position;
            
            switch(variant)		
            {
                case 0:
                {	
                    generate(8);
                    
                    break;
                }
                    
                case 1:
                {
                    generate(15);
                    
                    break;
                }
                    
                case 2:
                {
                    StackCursor = 0;
                    generate(32);
                    
                    break;
                }		
            }
            
            generate(47);
        }
        
        public override function onChannelMessage(channel:Channel, peer:Peer, subchannel:int, message:flash.utils.ByteArray, variant:int):void
        {
            SelectedChannel = channel;
            SelectedChannel.SelectedPeer = peer;
            
            IncomingSubchannel = subchannel;
            Received = message;
            ReceivedBegin = message.position;
            
            switch(variant)		
            {
                case 0:
                {	
                    generate(9);
                    break;
                }
                    
                case 1:
                {
                    generate(16);
                    break;
                }
                    
                case 2:
                {
                    StackCursor = 0;
                    generate(33);
                    
                    break;
                }	
            }				
            
            generate(48);
        }
        
        public override function onPeerMessage(channel:Channel, peer:Peer, subchannel:int, message:ByteArray, variant:int):void
        {
            SelectedChannel = channel;
            SelectedChannel.SelectedPeer = peer;
            
            IncomingSubchannel = subchannel;
            Received = message;
            ReceivedBegin = message.position;
            
            switch(variant)		
            {
                case 0:
                {	
                    generate(36);
                    break;
                }
                    
                case 1:
                {
                    generate(37);
                    break;
                }
                    
                case 2:
                {
                    StackCursor = 0;
                    generate(38);
                    
                    break;
                }		
            }
            
            generate(49);	
        }
        
		public function CRunLacewingGlobalData()
		{
			Objects = new Array();
			
			LastError = "No error";
			
            reset();
		}
        
        public function reset():void
        {
            loopName = "";
            
            Stack = new ByteArray;
            Stack.endian = flash.utils.Endian.LITTLE_ENDIAN;
            StackCursor = 0;
            
            WelcomeMessage = "";
            SelectedChannel = null;
        }
		
	}

}