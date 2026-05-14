namespace TRPG.Common
{
    /// <summary>
    /// CAUTION : Enum은 Flags 속성이 있어야 하며, int로 변환 가능한 타입
    /// </summary>
    public sealed class AtomicFlag<EnumType> where EnumType : struct, Enum
    {
        private int value;


        static AtomicFlag()
        {
            if (!Attribute.IsDefined(typeof(EnumType), typeof(FlagsAttribute)))
            {
                throw new InvalidOperationException($"{typeof(EnumType).Name} must have [Flags] attribute.");
            }
        }

        public AtomicFlag(EnumType initial = default)
        {
            value = Convert.ToInt32(initial);
        }


        /// <summary>
        /// true : 플래그 설정 성공
        /// false : 이미 해당 플래그가 설정O
        /// </summary>
        public bool TrySet(EnumType state)
        {
            int flag = Convert.ToInt32(state);

            while (true)
            {
                int snapshot = Volatile.Read(ref value);

                if ((snapshot & flag) != 0)
                    return false;

                int next = snapshot | flag;

                if (Interlocked.CompareExchange(ref value, next, snapshot) == snapshot)
                    return true;
            }
        }

        /// <summary>
        /// true : 플래그 해제 성공
        /// false : 이미 해당 플래그가 설정X
        /// </summary>
        public bool TryUnset(EnumType state)
        {
            int flag = Convert.ToInt32(state);

            while (true)
            {
                int snapshot = Volatile.Read(ref value);

                if ((snapshot & flag) == 0)
                    return false;

                int next = snapshot & ~flag;

                if (Interlocked.CompareExchange(ref value, next, snapshot) == snapshot)
                    return true;
            }
        }

        /// <summary>
        /// 지정한 플래그가 설정되어 있는지 확인
        /// </summary>
        public bool Has(EnumType state)
        {
            int snapshot = Volatile.Read(ref value);
            int flag = Convert.ToInt32(state);

            return (snapshot & flag) != 0;
        }

        /// <summary>
        /// 플래그 값을 int 원본 값으로
        /// </summary>
        public int Raw => Volatile.Read(ref value);
    }
}
