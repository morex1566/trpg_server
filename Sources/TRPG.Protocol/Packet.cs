using Google.FlatBuffers;

namespace TRPG.Protocol;

public readonly struct PacketReceiveContext<T> where T : struct, IFlatbufferObject
{

}

public readonly struct PacketSendContext<T> where T : struct, IFlatbufferObject
{

}