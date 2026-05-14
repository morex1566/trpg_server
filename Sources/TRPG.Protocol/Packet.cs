using System;
using Google.FlatBuffers;

namespace TRPG.Protocol;


/// <summary>
/// 패킷을 처리하는 공통 handle 타입
/// </summary>
public delegate void PacketHandler(in PacketContext context);

/// <summary>
/// 패킷 buffer holder. 런타임 큐와 송신 요청에서 사용
/// </summary>
public struct PacketContext
{
    /// <summary>
    /// 패킷을 수신한 connection (object로 순환참조 방지)
    /// </summary>
    public object? Owner;

    /// <summary>
    /// size-prefixed flatbuffers packet buffer
    /// </summary>
    public byte[] Buffer;


    public PacketContext(object owner, int bufferSize)
    {
        Owner = owner;
        Buffer = new byte[bufferSize];
    }

    public PacketContext(byte[] buffer)
    {
        Owner = null;
        Buffer = buffer;
    }


    public readonly Packet GetPacket()
    {
        var buffer = new ByteBuffer(Buffer);

        // size-prefixed: skip 4-byte prefix
        buffer.Position = sizeof(int);
        return Packet.GetRootAsPacket(buffer);
    }

    public readonly PayloadType GetPayloadType()
    {
        return GetPacket().PayloadType;
    }

    public readonly TPayload? PayloadAs<TPayload>() where TPayload : struct, IFlatbufferObject
    {
        return GetPacket().Payload<TPayload>();
    }

    /// <summary>
    /// payload 생성 함수를 사용해 송신 packet 생성
    /// </summary>
    public static PacketContext Create(PayloadType payloadType, Func<FlatBufferBuilder, int> createPayload)
    {
        var builder = new FlatBufferBuilder(256);

        int payloadOffset = createPayload(builder);
        var packet = Packet.CreatePacket(builder, payloadType, payloadOffset);

        builder.FinishSizePrefixed(packet.Value);

        byte[] buffer = builder.SizedByteArray();
        return new PacketContext(buffer);
    }
}
