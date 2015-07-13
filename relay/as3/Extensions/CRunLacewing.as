// ActionScript file

package Extensions
{
	import Actions.*;
	
	import Conditions.*;
	
	import Expressions.*;
	
	import Objects.*;
	
	import RunLoop.*;
	
	import Services.*;
	
	import Sprites.*;
	
	import flash.utils.ByteArray;
	
	import lacewing.Channel;
	import lacewing.ChannelListing;
	import lacewing.Client;
	import lacewing.Peer;
	
	public class CRunLacewing extends CRunExtension
	{	
        public var Message:ByteArray;
        
		public var globalAcrossSubapps:Boolean;
		public var globalID:String;
		public var global:Boolean;
		
   		public var data:CRunLacewingGlobalData;
				
		public function CRunLacewing()
		{
			
		}
	
		public override function getNumberOfConditions():int
		{
			return 65;
		}
		
		public override function destroyRunObject(bFast:Boolean):void
		{
			for (var ObjectIndex:int = 0; ObjectIndex < data.Objects.length; ++ ObjectIndex)
			{
				if (data.Objects[ObjectIndex] == this)
				{
					data.Objects.splice(ObjectIndex, 1);
					break;
				}
				
			}
		}
		
		override public function createRunObject(param1:CBinaryFile, param2:CCreateObjectInfo, param3:int) : Boolean
		{
            Message = new ByteArray();
            Message.endian = flash.utils.Endian.LITTLE_ENDIAN;
            param1.setUnicode(false);
			param1.skipBytes(5);
			
			var AutoClearStack:Boolean = param1.readByte() != 0;
			
			param1.skipBytes(1);
			
			globalAcrossSubapps = param1.readByte() != 0;
			globalID = param1.readStringSize(512);
			global = param1.readByte() != 0;
			
			if (!globalAcrossSubapps)
			{
				data = new CRunLacewingGlobalData();
				data.Objects.push(this);
            
				return true;
			}
			
			var rhrh:CRun = rh;
			
			while (rhrh.rhApp.parentApp)
				rhrh = rhrh.rhApp.parentApp.frame.rhPtr;
		
			var storage:CExtStorage = rhrh.getStorage(ho.hoIdentifier);
			var list:Object;
			
			if (!storage)
			{
				list = new Object();
				storage = new CExtStorage();
				
				storage.object = list;
				
				rhrh.addStorage(storage, ho.hoIdentifier);
			}
			
			list = storage.object;
			
			data = list[globalID];
			
			if (!data)
			{
				data = new CRunLacewingGlobalData();
				list[globalID] = data;
			}
			
			data.AutoClearStack = AutoClearStack;
			data.Objects.push(this);
			        
			return true;
		}
	
		// Conditions:
		public override function condition(num:int, cnd:CCndExtension):Boolean
		{
			switch (num)
			{
				case 59: // On channel listing loop with name %0
				{
					return (data.loopName.toLowerCase() == cnd.getParamExpString(rh, 0).toLowerCase());
				}
				
				case 60: // On channel listing loop finished with name %0
				{
					return (data.loopName.toLowerCase() == cnd.getParamExpString(rh, 0).toLowerCase());
				}
				
				case 61: // On channel peer loop with name %0
				{
					return (data.loopName.toLowerCase() == cnd.getParamExpString(rh, 0).toLowerCase());
				}
				
				case 62: // On channel peer loop finished with name %0
				{
					return (data.loopName.toLowerCase() == cnd.getParamExpString(rh, 0).toLowerCase());
				}
				
				case 63: // On joined channel loop with name %0
				{
					return (data.loopName.toLowerCase() == cnd.getParamExpString(rh, 0).toLowerCase());
				}
				
				case 64: // On joined channel loop finished with name %0
				{
					return (data.loopName.toLowerCase() == cnd.getParamExpString(rh, 0).toLowerCase());
				}
				
				case 57: // Peer is the channel master
					return cndPeer_is_the_channel_master(cnd);
				case 58: // You are the channel master
					return cndYou_are_the_channel_master(cnd);
					
				case 31: //Network_tools_ICMP_ping_On_ping_failure
					return cndNetwork_tools_ICMP_ping_On_ping_failure(cnd);
				case 30: //Network_tools_ICMP_ping_On_ping_success
					return cndNetwork_tools_ICMP_ping_On_ping_success(cnd);
				case 29: //Network_tools_LAN_enumeration_On_device
					return cndNetwork_tools_LAN_enumeration_On_device(cnd);
				case 56: //Lacewing_server_On_server_end
					return cndLacewing_server_On_server_end(cnd);
				case 55: //Lacewing_server_Server_is_running
					return cndLacewing_server_Server_is_running(cnd);
				case 52: //Message_Blasted_On_any_peer_message
					return cndMessage_Blasted_On_any_peer_message(cnd);
				case 41: //Message_Blasted_On_stack_peer_message
					return cndMessage_Blasted_On_stack_peer_message(cnd);
				case 40: //Message_Blasted_On_number_peer_message
					return cndMessage_Blasted_On_number_peer_message(cnd);
				case 39: //Message_Blasted_On_text_peer_message
					return cndMessage_Blasted_On_text_peer_message(cnd);
				case 51: //Message_Blasted_On_any_channel_message
					return cndMessage_Blasted_On_any_channel_message(cnd);
				case 35: //Message_Blasted_On_stack_channel_message
					return cndMessage_Blasted_On_stack_channel_message(cnd);
				case 23: //Message_Blasted_On_number_channel_message
					return cndMessage_Blasted_On_number_channel_message(cnd);
				case 22: //Message_Blasted_On_text_channel_message
					return cndMessage_Blasted_On_text_channel_message(cnd);
				case 50: //Message_Blasted_On_any_server_message
					return cndMessage_Blasted_On_any_server_message(cnd);
				case 34: //Message_Blasted_On_stack_server_message
					return cndMessage_Blasted_On_stack_server_message(cnd);
				case 21: //Message_Blasted_On_number_server_message
					return cndMessage_Blasted_On_number_server_message(cnd);
				case 20: //Message_Blasted_On_text_server_message
					return cndMessage_Blasted_On_text_server_message(cnd);
				case 49: //Message_Sent_On_any_peer_message
					return cndMessage_Sent_On_any_peer_message(cnd);
				case 38: //Message_Sent_On_stack_peer_message
					return cndMessage_Sent_On_stack_peer_message(cnd);
				case 37: //Message_Sent_On_number_peer_message
					return cndMessage_Sent_On_number_peer_message(cnd);
				case 36: //Message_Sent_On_text_peer_message
					return cndMessage_Sent_On_text_peer_message(cnd);
				case 48: //Message_Sent_On_any_channel_message
					return cndMessage_Sent_On_any_channel_message(cnd);
				case 33: //Message_Sent_On_stack_channel_message
					return cndMessage_Sent_On_stack_channel_message(cnd);
				case 16: //Message_Sent_On_number_channel_message
					return cndMessage_Sent_On_number_channel_message(cnd);
				case 9: //Message_Sent_On_text_channel_message
					return cndMessage_Sent_On_text_channel_message(cnd);
				case 47: //Message_Sent_On_any_server_message
					return cndMessage_Sent_On_any_server_message(cnd);
				case 32: //Message_Sent_On_stack_server_message
					return cndMessage_Sent_On_stack_server_message(cnd);
				case 15: //Message_Sent_On_number_server_message
					return cndMessage_Sent_On_number_server_message(cnd);
				case 8: //Message_Sent_On_text_server_message
					return cndMessage_Sent_On_text_server_message(cnd);
				case 54: //Name_Client_has_a_name
					return cndName_Client_has_a_name(cnd);
				case 53: //Name_On_name_changed
					return cndName_On_name_changed(cnd);
				case 7: //Name_On_name_denied
					return cndName_On_name_denied(cnd);
				case 6: //Name_On_name_set
					return cndName_On_name_set(cnd);
				case 45: //Channel_Peers_On_peer_changed_name
					return cndChannel_Peers_On_peer_changed_name(cnd);
				case 17: //Channel_Peers_On_loop_finished
					return cndChannel_Peers_On_loop_finished(cnd);
				case 13: //Channel_Peers_On_loop
					return cndChannel_Peers_On_loop(cnd);
				case 11: //Channel_Peers_On_peer_disconnect
					return cndChannel_Peers_On_peer_disconnect(cnd);
				case 10: //Channel_Peers_On_peer_connect
					return cndChannel_Peers_On_peer_connect(cnd);
				case 18: //Channel_On_joined_channel_loop_finished
					return cndChannel_On_joined_channel_loop_finished(cnd);
				case 14: //Channel_On_joined_channel_loop
					return cndChannel_On_joined_channel_loop(cnd);
				case 44: //Channel_On_leave_denied
					return cndChannel_On_leave_denied(cnd);
				case 43: //Channel_On_leave
					return cndChannel_On_leave(cnd);
				case 5: //Channel_On_denied
					return cndChannel_On_denied(cnd);
				case 4: //Channel_On_join
					return cndChannel_On_join(cnd);
				case 28: //Channel_listing_On_loop_finished
					return cndChannel_listing_On_loop_finished(cnd);
				case 27: //Channel_listing_On_loop
					return cndChannel_listing_On_loop(cnd);
				case 26: //Channel_listing_On_list_received
					return cndChannel_listing_On_list_received(cnd);
				case 3: //Connection_On_disconnect
					return cndConnection_On_disconnect(cnd);
				case 42: //Connection_Is_connected
					return cndConnection_Is_connected(cnd);
				case 2: //Connection_On_connection_denied
					return cndConnection_On_connection_denied(cnd);
				case 1: //Connection_On_connect
					return cndConnection_On_connect(cnd);
				case 0: //On_error
					return cndOn_error(cnd);
			}
			return false;
		}
	
		public function cndNetwork_tools_ICMP_ping_On_ping_failure(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndNetwork_tools_ICMP_ping_On_ping_success(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndNetwork_tools_LAN_enumeration_On_device(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndLacewing_server_On_server_end(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndLacewing_server_Server_is_running(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndMessage_Blasted_On_any_peer_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;
		} 
	
		public function cndMessage_Blasted_On_stack_peer_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Blasted_On_number_peer_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Blasted_On_text_peer_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Blasted_On_any_channel_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Blasted_On_stack_channel_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Blasted_On_number_channel_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Blasted_On_text_channel_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Blasted_On_any_server_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Blasted_On_stack_server_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Blasted_On_number_server_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Blasted_On_text_server_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Sent_On_any_peer_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Sent_On_stack_peer_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Sent_On_number_peer_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Sent_On_text_peer_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Sent_On_any_channel_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Sent_On_stack_channel_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Sent_On_number_channel_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Sent_On_text_channel_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Sent_On_any_server_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Sent_On_stack_server_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Sent_On_number_server_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndMessage_Sent_On_text_server_message(cnd:CCndExtension):Boolean
		{
			var Subchannel__for_any:int = cnd.getParamExpression(rh,0);
			return data.IncomingSubchannel == Subchannel__for_any || Subchannel__for_any == -1;;
		} 
	
		public function cndName_Client_has_a_name(cnd:CCndExtension):Boolean
		{
			return data.getName() != "";
		} 
	
		public function cndName_On_name_changed(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndName_On_name_denied(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndName_On_name_set(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_Peers_On_peer_changed_name(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_Peers_On_loop_finished(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_Peers_On_loop(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_Peers_On_peer_disconnect(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_Peers_On_peer_connect(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_On_joined_channel_loop_finished(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_On_joined_channel_loop(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_On_leave_denied(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_On_leave(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_On_denied(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_On_join(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_listing_On_loop_finished(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_listing_On_loop(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndChannel_listing_On_list_received(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndConnection_On_disconnect(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndConnection_Is_connected(cnd:CCndExtension):Boolean
		{
			return data.isConnected();
		} 
	
		public function cndConnection_On_connection_denied(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndConnection_On_connect(cnd:CCndExtension):Boolean
		{
			return true;
		} 
	
		public function cndOn_error(cnd:CCndExtension):Boolean
		{
			return true;
		}
		
		public function cndPeer_is_the_channel_master(cnd:CCndExtension):Boolean
		{
			if((!data.SelectedChannel) || (!(data.SelectedChannel.SelectedPeer)))
				return false;
			
			return data.SelectedChannel.SelectedPeer.isChannelMaster;
		}
		
		public function cndYou_are_the_channel_master(cnd:CCndExtension):Boolean
		{
			if(!data.SelectedChannel)
				return false;
			
			return data.SelectedChannel.isChannelMaster;
		}
		
		public override function action(num:int, act:CActExtension):void
		{
			var LoopName:String;
			var i:int;
		
			switch (num)
			{
				case 68: // Move received stack cursor to %0
				{
					data.StackCursor = act.getParamExpression(rh, 0);
					
					break;
				}
				
				case 69: // Loop listed channels with loop name %0
				{
					LoopName = act.getParamExpString(rh, 0);
					var channelList:Array = data.getChannelList();
                    
					for(i = 0; i < channelList.length; ++i)
					{
						data.CurrentChannelListing = channelList[i];
						
						data.loopName = LoopName;
						ho.generateEvent(59, 0);
					}
					
					ho.generateEvent(60, 0);
					break;
				}
				
				case 70: // Loop channels with loop name %0
				{
					LoopName = act.getParamExpString(rh, 0);
					var channels:Array = data.getChannels();
                    
					for(i = 0; i < channels.length; ++i)
					{
						data.SelectedChannel = channels[i];
						
						data.loopName = LoopName;
						ho.generateEvent(63, 0);
					}
					
					ho.generateEvent(64, 0);
			
					break;
				}
				
				case 71: // Loop peers on channel with loop name %0
				{
					if(!data.SelectedChannel)
						break;
						
					LoopName = act.getParamExpString(rh, 0);
						
					var Looping:Channel = data.SelectedChannel; 
					var Previous:Peer = data.SelectedChannel.SelectedPeer;
					
					for(i = 0; i < data.SelectedChannel.peers.length; ++i)
					{
						data.SelectedChannel.SelectedPeer = data.SelectedChannel.peers[i];
						
						data.loopName = LoopName;
						ho.generateEvent(63, 0);
						
						data.SelectedChannel = Looping;
					}
					
					data.SelectedChannel.SelectedPeer = Previous;
					ho.generateEvent(64, 0);
					
					break;
				}
                    
                case 73: // Connect (new)
                {
                    data.connect(act.getParamExpString(rh,0));
                    break;
                }
                    
				case 65: // Join channel (updated)
					return actJoin_channel(act);
				case 66: // Compress send data.Stack (ZLIB)
					return actCompress_send_stack_ZLIB(act);
				case 67: // Uncompress received data.Stack (ZLIB)
					return actUncompress_received_stack_ZLIB(act);
				case 64: // Select the channel master
					return actSelect_the_channel_master(act);
				
				case 33: //Network_tools_ICMP_ping_Ping_host
					return actNetwork_tools_ICMP_ping_Ping_host(act);
				case 32: //Network_tools_LAN_enumeration_Enumerate_devices
					return actNetwork_tools_LAN_enumeration_Enumerate_devices(act);
				case 63: //Lacewing_server_Reload_configuration_file
					return actLacewing_server_Reload_configuration_file(act);
				case 62: //Lacewing_server_Stop
					return actLacewing_server_Stop(act);
				case 61: //Lacewing_server_Start_with_LacewingServer_file
					return actLacewing_server_Start_with_LacewingServer_file(act);
				case 51: //Append_received_stack_to_file
					return actAppend_received_stack_to_file(act);
				case 50: //Save_received_stack_to_file
					return actSave_received_stack_to_file(act);
				case 49: //stack_Clear
					return actstack_Clear(act);
				case 52: //stack_Push_file
					return actstack_Push_file(act);
				case 48: //stack_Push_binary
					return actstack_Push_binary(act);
				case 47: //stack_Push_string_With_null_terminator
					return actstack_Push_string_With_null_terminator(act);
				case 46: //stack_Push_string_Without_null_terminator
					return actstack_Push_string_Without_null_terminator(act);
				case 45: //stack_Push_float
					return actstack_Push_float(act);
				case 44: //stack_Push_integer
					return actstack_Push_integer(act);
				case 43: //stack_Push_short
					return actstack_Push_short(act);
				case 42: //stack_Push_byte_Integer_value
					return actstack_Push_byte_Integer_value(act);
				case 41: //stack_Push_byte_ASCII_character
					return actstack_Push_byte_ASCII_character(act);
				case 40: //Blast_stack_To_peer
					return actBlast_stack_To_peer(act);
				case 39: //Blast_stack_To_channel
					return actBlast_stack_To_channel(act);
				case 38: //Blast_stack_To_server
					return actBlast_stack_To_server(act);
				case 16: //Blast_Number_To_peer
					return actBlast_Number_To_peer(act);
				case 15: //Blast_Number_To_channel
					return actBlast_Number_To_channel(act);
				case 14: //Blast_Number_To_server
					return actBlast_Number_To_server(act);
				case 13: //Blast_Text_To_peer
					return actBlast_Text_To_peer(act);
				case 12: //Blast_Text_To_channel
					return actBlast_Text_To_channel(act);
				case 11: //Blast_Text_To_server
					return actBlast_Text_To_server(act);
				case 37: //Send_stack_To_peer
					return actSend_stack_To_peer(act);
				case 36: //Send_stack_To_channel
					return actSend_stack_To_channel(act);
				case 35: //Send_stack_To_server
					return actSend_stack_To_server(act);
				case 10: //Send_Number_To_peer
					return actSend_Number_To_peer(act);
				case 9: //Send_Number_To_channel
					return actSend_Number_To_channel(act);
				case 8: //Send_Number_To_server
					return actSend_Number_To_server(act);
				case 7: //Send_Text_To_peer
					return actSend_Text_To_peer(act);
				case 6: //Send_Text_To_channel
					return actSend_Text_To_channel(act);
				case 5: //Send_Text_To_server
					return actSend_Text_To_server(act);
				case 22: //Channel_Peer_Loop
					return actChannel_Peer_Loop(act);
				case 21: //Channel_Peer_Select_by_ID
					return actChannel_Peer_Select_by_ID(act);
				case 20: //Channel_Peer_Select_by_name
					return actChannel_Peer_Select_by_name(act);
				case 4: //Channel_Leave
					return actChannel_Leave(act);
				case 3: //Channel_Join
					return actChannel_Join(act);
				case 19: //Channel_Select_Loop
					return actChannel_Select_Loop(act);
				case 17: //Channel_Select_By_name
					return actChannel_Select_By_name(act);
				case 31: //Channel_listing_Loop_listed_channels
					return actChannel_listing_Loop_listed_channels(act);
				case 30: //Channel_listing_Request_list
					return actChannel_listing_Request_list(act);
				case 2: //Set_name
					return actSet_name(act);
				case 1: //Disconnect
					return actDisconnect(act);
				case 0: //Connect
					return actConnect(act);
			}
		} 
	
		public function actNetwork_tools_ICMP_ping_Ping_host(act:CActExtension):void
		{
			var Timeout_ms:int = act.getParamExpression(rh,2);
			var Number_of_times_to_ping_host:int = act.getParamExpression(rh,1);
			var Host:String = act.getParamExpString(rh,0);
		} 
	
		public function actNetwork_tools_LAN_enumeration_Enumerate_devices(act:CActExtension):void
		{
		} 
	
		public function actLacewing_server_Reload_configuration_file(act:CActExtension):void
		{
		} 
	
		public function actLacewing_server_Stop(act:CActExtension):void
		{
		} 
	
		public function actLacewing_server_Start_with_LacewingServer_file(act:CActExtension):void
		{
			var Configuration_file_LacewingServerini:String = act.getParamExpString(rh,1);
			var LacewingServer_filename:String = act.getParamExpString(rh,0);
		} 
	
		public function actAppend_received_stack_to_file(act:CActExtension):void
		{
			var Filename:String = act.getParamExpString(rh,2);
			var Size:int = act.getParamExpression(rh,1);
			var Position:int = act.getParamExpression(rh,0);
		} 
	
		public function actSave_received_stack_to_file(act:CActExtension):void
		{
			var Filename:String = act.getParamExpString(rh,2);
			var Size:int = act.getParamExpression(rh,1);
			var Position:int = act.getParamExpression(rh,0);
		} 
	
		public function actstack_Clear(act:CActExtension):void
		{
			data.Stack = new ByteArray;
			data.Stack.endian=flash.utils.Endian.LITTLE_ENDIAN;
			
		} 
	
		public function actstack_Push_file(act:CActExtension):void
		{
			var File_to_push:String = act.getParamExpString(rh,0);
		} 
	
		public function actstack_Push_binary(act:CActExtension):void
		{
			var Size:int = act.getParamExpression(rh,1);
			var Address:int = act.getParamExpression(rh,0);
		}
	
		public function actstack_Push_string_With_null_terminator(act:CActExtension):void
		{
			var s:String = act.getParamExpString(rh,0);
			
			data.Stack.writeMultiByte(s, lacewing.Client.charset);
			data.Stack.writeByte(0);
		} 
	
		public function actstack_Push_string_Without_null_terminator(act:CActExtension):void
		{
			var s:String = act.getParamExpString(rh,0);
			
			data.Stack.writeMultiByte(s, lacewing.Client.charset);
		} 
	
		public function actstack_Push_float(act:CActExtension):void
		{
			// Uh maybe later
		} 
	
		public function actstack_Push_integer(act:CActExtension):void
		{
			var Integer:int = act.getParamExpression(rh,0);
			data.Stack.writeInt(Integer);
		} 
	
		public function actstack_Push_short(act:CActExtension):void
		{
			var Short:int = act.getParamExpression(rh,0);
			data.Stack.writeShort(Short);
		} 
	
		public function actstack_Push_byte_Integer_value(act:CActExtension):void
		{
			var Byte:int = act.getParamExpression(rh,0);
			data.Stack.writeByte(Byte);
		} 
	
		public function actstack_Push_byte_ASCII_character(act:CActExtension):void
		{
			var Byte:String = act.getParamExpString(rh,0);
			data.Stack.writeByte(Byte.charCodeAt(0));
		} 
	
		public function actBlast_stack_To_peer(act:CActExtension):void
		{
			var Subchannel:int = act.getParamExpression(rh,0);
		}
	
		public function actBlast_stack_To_channel(act:CActExtension):void
		{
			var Subchannel:int = act.getParamExpression(rh,0);
		} 
	
		public function actBlast_stack_To_server(act:CActExtension):void
		{
			var Subchannel:int = act.getParamExpression(rh,0);
		} 
	
		public function actBlast_Number_To_peer(act:CActExtension):void
		{
			var Number_to_send:int = act.getParamExpression(rh,1);
			var Subchannel_:int = act.getParamExpression(rh,0);
		} 
	
		public function actBlast_Number_To_channel(act:CActExtension):void
		{
			var Number_to_send:int = act.getParamExpression(rh,1);
			var Subchannel_:int = act.getParamExpression(rh,0);
		} 
	
		public function actBlast_Number_To_server(act:CActExtension):void
		{
			var Number_to_blast:int = act.getParamExpression(rh,1);
			var Subchannel_:int = act.getParamExpression(rh,0);
		} 
	
		public function actBlast_Text_To_peer(act:CActExtension):void
		{
			var Text_to_blast:String = act.getParamExpString(rh,1);
			var Subchannel_:int = act.getParamExpression(rh,0);
		} 
	
		public function actBlast_Text_To_channel(act:CActExtension):void
		{
			var Text_to_blast:String = act.getParamExpString(rh,1);
			var Subchannel_:int = act.getParamExpression(rh,0);
		} 
	
		public function actBlast_Text_To_server(act:CActExtension):void
		{
			var Text_to_blast:String = act.getParamExpString(rh,1);
			var Subchannel_:int = act.getParamExpression(rh,0);
		} 
	
		public function actSend_stack_To_peer(act:CActExtension):void
		{
			if((!data.SelectedChannel) || (!data.SelectedChannel.SelectedPeer))
				return;				
			
			var Subchannel:int = act.getParamExpression(rh,0);
			data.SelectedChannel.SelectedPeer.send(Subchannel, data.Stack, 2);
            
            if(data.AutoClearStack)
            {
                data.Stack.length = 0;
            }
		} 
	
		public function actSend_stack_To_channel(act:CActExtension):void
		{
			if(!data.SelectedChannel)
				return;
				
			var Subchannel:int = act.getParamExpression(rh,0);
			
			data.SelectedChannel.send(Subchannel, data.Stack, 2);
			
			if(data.AutoClearStack)
			{
				data.Stack.length = 0;
			}
		} 
	
		public function actSend_stack_To_server(act:CActExtension):void
		{
			var Subchannel:int = act.getParamExpression(rh,0);
			
			data.sendServer(Subchannel, data.Stack, 2);
			
			if(data.AutoClearStack)
			{
				data.Stack.length = 0;
			}
		} 
	
		public function actSend_Number_To_peer(act:CActExtension):void
		{
			if((!data.SelectedChannel) || (!data.SelectedChannel.SelectedPeer))
				return;
			
			var Number_to_send:int = act.getParamExpression(rh,1);
			var Subchannel_:int = act.getParamExpression(rh,0);
			
			Message.length = 0;
			Message.writeInt(Number_to_send);
			
			data.SelectedChannel.SelectedPeer.send(Subchannel_, Message, 1);
		} 
	
		public function actSend_Number_To_channel(act:CActExtension):void
		{
			if(!data.SelectedChannel)
				return;
				
			var Number_to_send:int = act.getParamExpression(rh,1);
			var Subchannel_:int = act.getParamExpression(rh,0);
			
			Message.length = 0;
			Message.writeInt(Number_to_send);
			
			data.SelectedChannel.send(Subchannel_, Message, 1);
		} 
	
		public function actSend_Number_To_server(act:CActExtension):void
		{
			var Number_to_send:int = act.getParamExpression(rh,1);
			var Subchannel_:int = act.getParamExpression(rh,0);
			
			Message.length = 0;
			Message.writeInt(Number_to_send);
			
			data.sendServer(Subchannel_, Message, 1);
		} 
	
		public function actSend_Text_To_peer(act:CActExtension):void
		{
			if((!data.SelectedChannel) || (!data.SelectedChannel.SelectedPeer))
				return;				
			
			var Text_to_send:String = act.getParamExpString(rh,1);
			var Subchannel_:int = act.getParamExpression(rh,0);
			
			Message.length = 0;
			Message.writeMultiByte(Text_to_send, lacewing.Client.charset);
			
			data.SelectedChannel.SelectedPeer.send(Subchannel_, Message, 0);
		} 
	
		public function actSend_Text_To_channel(act:CActExtension):void
		{
			if(!data.SelectedChannel)
				return;
				
			var Text_to_send:String = act.getParamExpString(rh,1);
			var Subchannel_:int = act.getParamExpression(rh,0);
			
			Message.length = 0;
			Message.writeMultiByte(Text_to_send, lacewing.Client.charset);
			
			data.SelectedChannel.send(Subchannel_, Message, 0);
		} 
	
		public function actSend_Text_To_server(act:CActExtension):void
		{
			var Text_to_send:String = act.getParamExpString(rh,1);
			var Subchannel_:int = act.getParamExpression(rh,0);
			
			Message.length = 0;
			Message.writeMultiByte(Text_to_send, lacewing.Client.charset);
			
			data.sendServer(Subchannel_, Message, 0);
		} 
	
		public function actChannel_Peer_Loop(act:CActExtension):void
		{
			if(!data.SelectedChannel)
				return;
				
			var Looping:Channel = data.SelectedChannel; 
			var Previous:Peer = data.SelectedChannel.SelectedPeer;
			
			for(var i:int = 0; i < data.SelectedChannel.peers.length; ++i)
			{
				data.SelectedChannel.SelectedPeer = data.SelectedChannel.peers[i];
				
				ho.generateEvent(13, 0);
				
				data.SelectedChannel = Looping;
			}
			
			data.SelectedChannel.SelectedPeer = Previous;
			ho.generateEvent(17, 0);
		} 
					
		public function actChannel_Peer_Select_by_ID(act:CActExtension):void
		{
			if(!data.SelectedChannel)
				return;
			
			var ID:int = act.getParamExpression(rh,0);
			
			for(var i:int = 0; i < data.SelectedChannel.peers.length; ++i)
			{
				if(data.SelectedChannel.peers[i].ID == ID)
				{
					data.SelectedChannel.SelectedPeer = data.SelectedChannel.peers[i];
				}	
			}
		} 
	
		public function actChannel_Peer_Select_by_name(act:CActExtension):void
		{
			if(!data.SelectedChannel)
				return;
				
			var Name:String = act.getParamExpString(rh,0);
			
			for(var i:int = 0; i < data.SelectedChannel.peers.length; ++i)
			{
				if(data.SelectedChannel.peers[i].name == Name)
				{
					data.SelectedChannel.SelectedPeer = data.SelectedChannel.peers[i];
				}	
			}
		} 
	
		public function actChannel_Leave(act:CActExtension):void
		{
			if(!data.SelectedChannel)
				return;
				
			data.SelectedChannel.leave();
		} 
	
		public function actChannel_Join(act:CActExtension):void
		{
			var Hide_channel_from_list_if_creating___no___yes:int = act.getParamExpression(rh,1);
			var Channel_name:String = act.getParamExpString(rh,0);
			
			data.joinChannel(Channel_name, Hide_channel_from_list_if_creating___no___yes != 0);
		} 
	
		public function actChannel_Select_Loop(act:CActExtension):void
		{
            var channels:Array = data.getChannels();
            
			for(var i:int = 0; i < channels.length; ++i)
			{
				data.SelectedChannel = channels[i];
				ho.generateEvent(14, 0);
			}
			
			ho.generateEvent(18, 0);
		}
	
		public function actChannel_Select_By_name(act:CActExtension):void
		{
			var Name:String = act.getParamExpString(rh,0);
		} 
	
		public function actChannel_listing_Loop_listed_channels(act:CActExtension):void
		{
            var channelList:Array = data.getChannelList();
            
			for(var i:int = 0; i < channelList.length; ++i)
			{
				data.CurrentChannelListing = channelList[i];
				
				ho.generateEvent(27, 0);
			}
			
			ho.generateEvent(28, 0);
		}
	
		public function actChannel_listing_Request_list(act:CActExtension):void
		{
			data.requestChannelList();
		} 
	
		public function actSet_name(act:CActExtension):void
		{
			var Name:String = act.getParamExpString(rh,0);
			
			data.setName(Name);
		} 
	
		public function actDisconnect(act:CActExtension):void
		{
			data.disconnect();
		} 
	
		public function actConnect(act:CActExtension):void
		{
			data.connect(act.getParamExpString(rh,0), act.getParamExpression(rh,1));
		} 
		
		public function actCompress_send_stack_ZLIB(act:CActExtension):void
		{

		}
	
		public function actUncompress_received_stack_ZLIB(act:CActExtension):void
		{

		}
	

		public function actSelect_the_channel_master(act:CActExtension):void
		{
			if(!data.SelectedChannel)
				return;
			
			for(var i:int = 0; i < data.SelectedChannel.peers.length; ++i)
			{
				data.SelectedChannel.SelectedPeer = data.SelectedChannel.peers[i];
				
				if(data.SelectedChannel.SelectedPeer.isChannelMaster)
				{
					return;
				}	
			}
			
			data.SelectedChannel.SelectedPeer = null;
			return;
		}
		
		public function actJoin_channel(act:CActExtension):void
		{
			var Hide_channel_from_list_if_creating___no___yes:int = act.getParamExpression(rh,1);
			var Channel_name:String = act.getParamExpString(rh,0);
			var Close_automatically:int = act.getParamExpression(rh, 2);
			
			data.joinChannel(Channel_name, Hide_channel_from_list_if_creating___no___yes != 0, Close_automatically != 0);
			return;
		}
		
		// Expressions:
		public override function expression(num:int):CValue
		{
			switch (num)
			{
				case 42: // Get stack memory address
				{
					return new CValue(0);
				}
				
				case 52: //Received_Get_stack_data_String_Null_terminated
					return expReceived_Get_stack_data_String_Null_terminated_cursor();
					
				case 53:
				{
					// Server protocol implementation
					
                    var ret:CValue = new CValue(0);
                    ret.forceString("");
					return ret;
				}
				
				case 51: //Received_Get_stack_data_String_With_size
					return expReceived_Get_stack_data_String_With_size_cursor();
				case 50: //Received_Get_stack_data_Float
					return expReceived_Get_stack_data_Float_cursor();
				case 49: //Received_Get_stack_data_Integer_Signed
					return expReceived_Get_stack_data_Integer_Signed_cursor();
				case 48: //Received_Get_stack_data_Integer_Unsigned
					return expReceived_Get_stack_data_Integer_Unsigned_cursor();
				case 47: //Received_Get_stack_data_Short_Signed
					return expReceived_Get_stack_data_Short_Signed_cursor();
				case 46: //Received_Get_stack_data_Short_Unsigned
					return expReceived_Get_stack_data_Short_Unsigned_cursor();
				case 45: //Received_Get_stack_data_Byte_Integer_value_Signed
					return expReceived_Get_stack_data_Byte_Integer_value_Signed_cursor();
				case 44: //Received_Get_stack_data_Byte_Integer_value_Unsigned
					return expReceived_Get_stack_data_Byte_Integer_value_Unsigned_cursor();
				case 43: //Received_Get_stack_data_Byte_ASCII_character
					return expReceived_Get_stack_data_Byte_ASCII_character_cursor();
					

				
				case 19: //ICMP_ping_Requests_left
					return expICMP_ping_Requests_left();
				case 18: //ICMP_ping_Time_to_live
					return expICMP_ping_Time_to_live();
				case 17: //ICMP_ping_Roundtrip_time
					return expICMP_ping_Roundtrip_time();
				case 16: //LAN_enumeration_Device_IP_address
					return expLAN_enumeration_Device_IP_address();
				case 15: //LAN_enumeration_Device_hostname
					return expLAN_enumeration_Device_hostname();
				case 40: //Lacewing_server_Get_version
					return expLacewing_server_Get_version();
				case 34: //Selected_channel_Selected_peer_Previous_name_for_on_peer_name_changed
					return expSelected_channel_Selected_peer_Previous_name_for_on_peer_name_changed();
				case 8: //Selected_channel_Selected_peer_ID
					return expSelected_channel_Selected_peer_ID();
				case 4: //Selected_channel_Selected_peer_Name
					return expSelected_channel_Selected_peer_Name();
				case 10: //Selected_channel_Number_of_peers
					return expSelected_channel_Number_of_peers();
				case 9: //Selected_channel_Name
					return expSelected_channel_Name();
				case 33: //Self_Previous_name_for_on_name_changed
					return expSelf_Previous_name_for_on_name_changed();
				case 35: //Self_Connection_time
					return expSelf_Connection_time();
				case 3: //Self_Number_of_channels
					return expSelf_Number_of_channels();
				case 14: //Self_ID
					return expSelf_ID();
				case 2: //Self_Name
					return expSelf_Name();
				case 7: //Received_Get_subchannel
					return expReceived_Get_subchannel();
				case 29: //Received_Get_stack_data_String_Null_terminated
					return expReceived_Get_stack_data_String_Null_terminated();
				case 28: //Received_Get_stack_data_String_With_size
					return expReceived_Get_stack_data_String_With_size();
				case 27: //Received_Get_stack_data_Float
					return expReceived_Get_stack_data_Float();
				case 26: //Received_Get_stack_data_Integer_Signed
					return expReceived_Get_stack_data_Integer_Signed();
				case 25: //Received_Get_stack_data_Integer_Unsigned
					return expReceived_Get_stack_data_Integer_Unsigned();
				case 24: //Received_Get_stack_data_Short_Signed
					return expReceived_Get_stack_data_Short_Signed();
				case 23: //Received_Get_stack_data_Short_Unsigned
					return expReceived_Get_stack_data_Short_Unsigned();
				case 22: //Received_Get_stack_data_Byte_Integer_value_Signed
					return expReceived_Get_stack_data_Byte_Integer_value_Signed();
				case 21: //Received_Get_stack_data_Byte_Integer_value_Unsigned
					return expReceived_Get_stack_data_Byte_Integer_value_Unsigned();
				case 20: //Received_Get_stack_data_Byte_ASCII_character
					return expReceived_Get_stack_data_Byte_ASCII_character();
				case 30: //Received_Get_stack_data_Size
					return expReceived_Get_stack_data_Size();
				case 6: //Received_Get_number
					return expReceived_Get_number();
				case 5: //Received_Get_text
					return expReceived_Get_text();
				case 13: //Channel_listing_Get_peer_count
					return expChannel_listing_Get_peer_count();
				case 12: //Channel_listing_Get_name
					return expChannel_listing_Get_name();
				case 41: //Get_welcome_message_for_on_connect
					return expGet_welcome_message_for_on_connect();
				case 39: //Connection_Host_port
					return expConnection_Host_port();
				case 38: //Connection_Host_IP_address
					return expConnection_Host_IP_address();
				case 37: //Deny_reason_for_on__denied
					return expDeny_reason_for_on__denied();
				case 32: //Send_stack_size
					return expSend_stack_size();
				case 31: //Lacewing_version_string
					return expLacewing_version_string();
				case 0: //Error_string_for_on_error
					return expError_string_for_on_error();
			}
			
			return new CValue(0);
		}
	
		public function expICMP_ping_Requests_left():CValue
		{
			return new CValue(0);
		} 
	
		public function expICMP_ping_Time_to_live():CValue
		{
			return new CValue(0);
		} 
	
		public function expICMP_ping_Roundtrip_time():CValue
		{
			return new CValue(0);
		} 
	
		public function expLAN_enumeration_Device_IP_address():CValue
		{
			var Return:CValue;
			Return.forceString("");
			return Return;
		} 
	
		public function expLAN_enumeration_Device_hostname():CValue
		{
			var Return:CValue;
			Return.forceString("");
			return Return;
		} 
	
		public function expLacewing_server_Get_version():CValue
		{
			var Return:CValue;
			Return.forceString("");
			return Return;
		} 
	
		public function expSelected_channel_Selected_peer_Previous_name_for_on_peer_name_changed():CValue
		{
			var Return:CValue;
			Return.forceString(data.PeerPreviousName);
			return Return;
		} 
	
		public function expSelected_channel_Selected_peer_ID():CValue
		{
            try
            {
			    return new CValue(data.SelectedChannel.SelectedPeer.id);
            }
            catch(e:Error)
            {
            }
            
            return new CValue(0);
		} 
	
		public function expSelected_channel_Selected_peer_Name():CValue
		{
			var Return:CValue = new CValue(0);
            Return.forceString("");
            
            try
            {
			    Return.forceString(data.SelectedChannel.SelectedPeer.name);
            }
            catch(e:Error)
            {
            }
            
			return Return;
		} 
	
		public function expSelected_channel_Number_of_peers():CValue
		{
			if(data.SelectedChannel == null)
			{
				return new CValue(0);
			}
				
			return new CValue(data.SelectedChannel.peers.length);
		} 
	
		public function expSelected_channel_Name():CValue
		{
			var Return:CValue = new CValue(0);
			
			if(data.SelectedChannel)
			{
				Return.forceString(data.SelectedChannel.name);
			}
			else
			{
				Return.forceString("");
			}
			
			return Return;
		} 
	
		public function expSelf_Previous_name_for_on_name_changed():CValue
		{
			var Return:CValue = new CValue(0);
			Return.forceString(data.PreviousName);
			return Return;
		} 
	
		public function expSelf_Connection_time():CValue
		{
			return new CValue(-1);
		} 
	
		public function expSelf_Number_of_channels():CValue
		{
			return new CValue(data.getChannels().length);
		} 
	
		public function expSelf_ID():CValue
		{
			return new CValue(data.getID());
		} 
	
		public function expSelf_Name():CValue
		{
			var Return:CValue = new CValue(0);
			Return.forceString(data.getName());
			return Return;
		} 
	
		public function expReceived_Get_subchannel():CValue
		{
			return new CValue(data.IncomingSubchannel);
		} 
	
		public function expReceived_Get_stack_data_String_Null_terminated():CValue
		{
			var Index:int = ho.getExpParam().getInt();
			
			var Value:String = "";
			
			data.Received.position = data.ReceivedBegin + Index;
			
			if(!data.Received.bytesAvailable)
				return new CValue(0);
			
			var Byte:int = data.Received.readByte();
			
			while(Byte != 0 && data.Received.bytesAvailable)
				Byte = data.Received.readByte();
			
			var Size:int = data.Received.position - (data.ReceivedBegin + Index);
			data.Received.position = data.ReceivedBegin + Index;
			
			var Return:CValue = new CValue(0);
			Return.forceString(data.Received.readMultiByte(Size, lacewing.Client.charset));
			return Return;
		} 
	
		public function expReceived_Get_stack_data_String_With_size():CValue
		{
			var Size:int = ho.getExpParam().getInt();
			var Index:int = ho.getExpParam().getInt();
			
			data.Received.position = data.ReceivedBegin + Index;
			
			var Return:CValue = new CValue(0);
			if(data.Received.bytesAvailable >= Size)
				Return.forceString(data.Received.readMultiByte(Size, lacewing.Client.charset));
			return Return;
		} 
	
		public function expReceived_Get_stack_data_Float():CValue
		{
			var Index:int = ho.getExpParam().getInt();
			return new CValue(1);
		} 
	
		public function expReceived_Get_stack_data_Integer_Signed():CValue
		{
			var Index:int = ho.getExpParam().getInt();
			data.Received.position = data.ReceivedBegin + Index;
			if(data.Received.bytesAvailable < 4)
				return new CValue(0);
			return new CValue(data.Received.readInt());
		} 
	
		public function expReceived_Get_stack_data_Integer_Unsigned():CValue
		{
			var Index:int = ho.getExpParam().getInt();
			data.Received.position = data.ReceivedBegin + Index;
			if(data.Received.bytesAvailable < 4)
				return new CValue(0);
			return new CValue(data.Received.readUnsignedInt());
		} 
	
		public function expReceived_Get_stack_data_Short_Signed():CValue
		{
			var Index:int = ho.getExpParam().getInt();
			data.Received.position = data.ReceivedBegin + Index;
			if(data.Received.bytesAvailable < 2)
				return new CValue(0);
			return new CValue(data.Received.readShort());
		} 
	
		public function expReceived_Get_stack_data_Short_Unsigned():CValue
		{
			var Index:int = ho.getExpParam().getInt();
			data.Received.position = data.ReceivedBegin + Index;
			if(data.Received.bytesAvailable < 2)
				return new CValue(0);
			return new CValue(data.Received.readUnsignedShort());
		} 
	
		public function expReceived_Get_stack_data_Byte_Integer_value_Signed():CValue
		{
			var Index:int = ho.getExpParam().getInt();
			data.Received.position = data.ReceivedBegin + Index;
			if(!data.Received.bytesAvailable)
				return new CValue(0);
			return new CValue(data.Received.readByte());
		} 
	
		public function expReceived_Get_stack_data_Byte_Integer_value_Unsigned():CValue
		{
			var Index:int = ho.getExpParam().getInt();
			data.Received.position = data.ReceivedBegin + Index;
			if(!data.Received.bytesAvailable)
				return new CValue(0);
			return new CValue(data.Received.readUnsignedByte());
		} 
	
		public function expReceived_Get_stack_data_Byte_ASCII_character():CValue
		{
			var Index:int = ho.getExpParam().getInt();
			data.Received.position = data.ReceivedBegin + Index;
			var Return:CValue = new CValue(0);
			if(data.Received.bytesAvailable)
				Return.forceString(data.Received.readMultiByte(1, lacewing.Client.charset));
			return Return;
		} 
		public function expReceived_Get_stack_data_String_Null_terminated_cursor():CValue
		{
			var Index:int = data.StackCursor;
			
			var Value:String = "";
			
			data.Received.position = data.ReceivedBegin + Index;
			
			
			if(!data.Received.bytesAvailable)
				return new CValue(0);
			
			var Byte:int = data.Received.readByte();
			
			while(Byte != 0 && data.Received.bytesAvailable)
				Byte = data.Received.readByte();
			
			var Size:int = data.Received.position - (data.ReceivedBegin + Index);
			data.Received.position = data.ReceivedBegin + Index;
		
			var Return:CValue = new CValue(0);
			Return.forceString(data.Received.readMultiByte(Size, lacewing.Client.charset));

			data.StackCursor += Size;

			return Return;
		} 
	
		public function expReceived_Get_stack_data_String_With_size_cursor():CValue
		{
			var Size:int = ho.getExpParam().getInt();
			var Index:int = data.StackCursor;
			
			data.Received.position = data.ReceivedBegin + Index;
			
			var Return:CValue = new CValue(0);
			if(data.Received.bytesAvailable >= Size)
			{
				Return.forceString(data.Received.readMultiByte(Size, lacewing.Client.charset));
				data.StackCursor += Size;
			}


			return Return;
		} 
	
		public function expReceived_Get_stack_data_Float_cursor():CValue
		{
			var Index:int = data.StackCursor;
			return new CValue(1);
		} 
	
		public function expReceived_Get_stack_data_Integer_Signed_cursor():CValue
		{
			var Index:int = data.StackCursor;
			
			data.Received.position = data.ReceivedBegin + Index;
			if(data.Received.bytesAvailable < 4)
				return new CValue(0);
			
			data.StackCursor += 4;

			return new CValue(data.Received.readInt());
		} 
	
		public function expReceived_Get_stack_data_Integer_Unsigned_cursor():CValue
		{
			var Index:int = data.StackCursor;
			
			data.Received.position = data.ReceivedBegin + Index;
			if(data.Received.bytesAvailable < 4)
				return new CValue(0);

			data.StackCursor += 4;

			return new CValue(data.Received.readUnsignedInt());
		} 
	
		public function expReceived_Get_stack_data_Short_Signed_cursor():CValue
		{
			var Index:int = data.StackCursor;
			
			data.Received.position = data.ReceivedBegin + Index;
			
			if(data.Received.bytesAvailable < 2)
				return new CValue(0);
			
			data.StackCursor += 2;

			return new CValue(data.Received.readShort());
		} 
	
		public function expReceived_Get_stack_data_Short_Unsigned_cursor():CValue
		{
			var Index:int = data.StackCursor;
			
			
			data.Received.position = data.ReceivedBegin + Index;
			
			if(data.Received.bytesAvailable < 2)
				return new CValue(0);
			
			data.StackCursor += 2;

			return new CValue(data.Received.readUnsignedShort());
		} 
	
		public function expReceived_Get_stack_data_Byte_Integer_value_Signed_cursor():CValue
		{
			var Index:int = data.StackCursor;
		
			
			data.Received.position = data.ReceivedBegin + Index;
			
			if(data.Received.bytesAvailable < 1)
				return new CValue(0);
			
			data.StackCursor += 1;

			return new CValue(data.Received.readByte());
		} 
	
		public function expReceived_Get_stack_data_Byte_Integer_value_Unsigned_cursor():CValue
		{
			var Index:int = data.StackCursor;
			
			data.Received.position = data.ReceivedBegin + Index;
			
			if(data.Received.bytesAvailable < 1)
				return new CValue(0);
			
			data.StackCursor += 1;

			return new CValue(data.Received.readUnsignedByte());
		} 
	
		public function expReceived_Get_stack_data_Byte_ASCII_character_cursor():CValue
		{
			var Index:int = data.StackCursor;
		
			
			data.Received.position = data.ReceivedBegin + Index;
			if(data.Received.bytesAvailable < 1)
				return new CValue(0);
			var Return:CValue = new CValue(0);
			Return.forceString(data.Received.readMultiByte(1, lacewing.Client.charset));

			data.StackCursor += 1;

			return Return;
		} 
		
		public function expReceived_Get_stack_data_Size():CValue
		{
			return new CValue(data.Received.length);
		} 
	
		public function expReceived_Get_number():CValue
		{
			data.Received.position = data.ReceivedBegin;
			return new CValue(data.Received.readInt());
		} 
	
		public function expReceived_Get_text():CValue
		{
			data.Received.position = data.ReceivedBegin;
			
			var Return:CValue = new CValue(0);
			Return.forceString(data.Received.readMultiByte(data.Received.length - data.Received.position, lacewing.Client.charset));
			return Return;
		} 
	
		public function expChannel_listing_Get_peer_count():CValue
		{
			if(data.CurrentChannelListing)
			{
				return new CValue(data.CurrentChannelListing.peerCount);			
			}
			else
			{
				return new CValue(-1);	
			}
		} 
	
		public function expChannel_listing_Get_name():CValue
		{
			var Return:CValue = new CValue(0);
			Return.forceString(data.CurrentChannelListing.name);
			return Return;
		} 
	
		public function expGet_welcome_message_for_on_connect():CValue
		{
			var Return:CValue = new CValue(0);
			Return.forceString(data.WelcomeMessage);
			return Return;
		} 
	
		public function expConnection_Host_port():CValue
		{
			return new CValue(data.getPort());
		} 
	
		public function expConnection_Host_IP_address():CValue
		{
			var Return:CValue = new CValue(0);
			Return.forceString("");
			return Return;
		} 
	
		public function expDeny_reason_for_on__denied():CValue
		{
			var Return:CValue = new CValue(0);
			Return.forceString(data.DenyReason);
			return Return;
		} 
	
		public function expSend_stack_size():CValue
		{
			return new CValue(data.Stack.length);
		} 
	
		public function expLacewing_version_string():CValue
		{
			var Return:CValue = new CValue(0);
			Return.forceString("Build #18 (SWF) Villy's hack Build #4");
			return Return;
		} 
	
		public function expError_string_for_on_error():CValue
		{
			var Return:CValue = new CValue(0);
			Return.forceString(data.LastError);
			return Return;
		}

	}

}

