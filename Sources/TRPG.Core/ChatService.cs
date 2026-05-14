using TRPG.Protocol;

namespace TRPG.Core
{
    public static partial class ChatService
    {
        [NetworkingMethod.Receive(PayloadType.ChatMessageRequest)]
        public static void OnRequestChatMessage(in PacketContext context)
        {
            
        }

        [NetworkingMethod.Send(PayloadType.ChatMessageResponse)]
        public static void OnResponseChatMessage(ref PacketContext context)
        {

        }
    }
}
