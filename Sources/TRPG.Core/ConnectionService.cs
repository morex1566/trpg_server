using TRPG.Networking;
using TRPG.Protocol;

namespace TRPG.Core
{
    public static partial class ConnectionService
    {
        [NetworkingMethod.Receive(PayloadType.ConnectRequest)]
        public static void RequestConnect(ref PacketContext context)
        {
            ResponseConnect(ref context);
        }

        [NetworkingMethod.Send(PayloadType.ConnectResponse)]
        public static PacketContext OnResponseConnect(ref PacketContext context)
        {
            Console.WriteLine($"Received connect request: {context.Owner!.Guid}");

            ulong guid = context.Owner!.Guid;

            return PacketContext.Create(PayloadType.ConnectResponse,
            builder =>
            {
                return ConnectResponse.CreateConnectResponse(builder, guid).Value;
            });
        }
    }
}
