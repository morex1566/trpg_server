using TRPG.Networking;
using TRPG.Protocol;

namespace TRPG.Core
{
    public static partial class ChatService
    {
        [NetworkingMethod.Receive(PayloadType.ChatMessageRequest)]
        public static void RequestChatMessage(ref PacketContext context)
        {
            ResponseChatMessage(ref context);
        }

        [NetworkingMethod.Send(PayloadType.ChatMessageResponse)]
        public static PacketContext OnResponseChatMessage(ref PacketContext context)
        {
            ChatMessageRequest request = context.PayloadAs<ChatMessageRequest>()!.Value;
            Console.WriteLine($"Received chat message: {request.Message}");

            string message = request.Message + " by server";

            return PacketContext.Create(PayloadType.ChatMessageResponse,
            builder =>
            {
                var messageOffset = builder.CreateString(message);
                return ChatMessageResponse.CreateChatMessageResponse(builder, messageOffset).Value;
            });
        }
    }
}
