using System.Collections;
using System.Numerics;
using System.Reflection;
using System.Text;

namespace ModelProcess
{
    public static class SerializeExtension
    {
        public static void Serialize<T>(this CustomedBinaryWriter writer, T value)
        {
            if (value is UInt64 u64)
            {
                writer.Write(u64);
            }
            else if (value is Int64 i64)
            {
                writer.Write(i64);
            }
            else if (value is UInt32 u32)
            {
                writer.Write(u32);
            }
            else if (value is Int32 i32)
            {
                writer.Write(i32);
            }
            else if (value is double d)
            {
                writer.Write(d);
            }
            else if (value is float f)
            {
                writer.Write(f);
            }
            else if (value is Vector2 v2)
            {
                writer.Write(v2.X);
                writer.Write(v2.Y);
            }
            else if (value is Vector3 v3)
            {
                writer.Write(v3.X);
                writer.Write(v3.Y);
                writer.Write(v3.Z);
            }
            else if (value is Vector4 v4)
            {
                writer.Write(v4.X);
                writer.Write(v4.Y);
                writer.Write(v4.Z);
                writer.Write(v4.W);
            }
            else if (value is string s)
            {
                writer.Write(s);
            }
            else if (value is Guid guid)
            {
                writer.Write(guid.ToByteArray());
            }
            else if (value is IDictionary dict)
            {
                writer.Write(dict.Cast<object>().Count());
                foreach (DictionaryEntry entry in dict)
                {
                    writer.Serialize(entry.Key);
                    writer.Serialize(entry.Value);
                }
            }
            else if (value is IEnumerable enumerable)
            {
                writer.Write(enumerable.Cast<object>().Count());
                foreach (var item in enumerable)
                {
                    writer.Serialize(item);
                }
            }
            else if(value.GetType().GetCustomAttribute<ByteSerializableAttribute>() != null)
            {
                var members = value.GetType()
                    .GetMembers(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance)
                    .Where(m => m.MemberType == MemberTypes.Field || m.MemberType == MemberTypes.Property)
                    .OrderBy(m => m.MetadataToken);

                foreach (var member in members)
                {
                    object memberValue = member.MemberType switch
                    {
                        MemberTypes.Field => ((FieldInfo)member).GetValue(value),
                        MemberTypes.Property => ((PropertyInfo)member).GetValue(value),
                    };
                    writer.Serialize(memberValue);
                }
            }
            else
            {
                throw new NotImplementedException($"Not implemented type {value.GetType().FullName}");
            }
        }
    }

    public class CustomedBinaryWriter : BinaryWriter
    {
        protected static Encoding encoding = Encoding.UTF8;

        public CustomedBinaryWriter(Stream stream)
            : base(stream, encoding)
        {
        }

        public override void Write(string value)
        {
            ArgumentNullException.ThrowIfNull(value);

            int actualBytecount = encoding.GetByteCount(value);
            Write(actualBytecount);
            Write(value.ToCharArray());
        }
    }

    [AttributeUsage(AttributeTargets.Struct | AttributeTargets.Class)]
    public class ByteSerializableAttribute : Attribute
    {

    }
}
