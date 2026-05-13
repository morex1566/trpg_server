using System;
using TRPG.Protocol;

namespace TRPG.Core;

public static class NetworkingMethod
{
    [AttributeUsage(AttributeTargets.Method)]
    public sealed class ReceiveAttribute : Attribute
    {
        public PayloadType type;



        private ReceiveAttribute() { }

        public ReceiveAttribute(PayloadType type)
        {
            this.type = type;
        }
    }

    [AttributeUsage(AttributeTargets.Method)]
    public sealed class SendAttribute : Attribute
    {
        public PayloadType type;



        private SendAttribute() { }

        public SendAttribute(PayloadType type)
        {
            this.type = type;
        }
    }
}