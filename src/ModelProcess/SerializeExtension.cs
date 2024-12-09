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

        public static T Deserialize<T>(this CustomedBinaryReader reader)
        {
            var thisMethod = typeof(SerializeExtension).GetMethod(nameof(SerializeExtension.Deserialize));

            var type = typeof(T);

            if (type == typeof(UInt64))
            {
                return (T)(object)reader.ReadUInt64();
            }
            else if (type == typeof(Int64))
            {
                return (T)(object)reader.ReadInt64();
            }
            else if (type == typeof(UInt32))
            {
                return (T)(object)reader.ReadUInt32();
            }
            else if (type == typeof(Int32))
            {
                return (T)(object)reader.ReadInt32();
            }
            else if (type == typeof(double))
            {
                return (T)(object)reader.ReadDouble();
            }
            else if (type == typeof(float))
            {
                return (T)(object)reader.ReadSingle();
            }
            else if (type == typeof(Vector2))
            {
                return (T)(object)new Vector2(reader.ReadSingle(), reader.ReadSingle());
            }
            else if (type == typeof(Vector3))
            {
                return (T)(object)new Vector3(reader.ReadSingle(), reader.ReadSingle(), reader.ReadSingle());
            }
            else if (type == typeof(Vector4))
            {
                return (T)(object)new Vector4(reader.ReadSingle(), reader.ReadSingle(), reader.ReadSingle(), reader.ReadSingle());
            }
            else if (type == typeof(string))
            {
                return (T)(object)reader.ReadString();
            }
            else if (type == typeof(Guid))
            {
                return (T)(object)new Guid(reader.ReadBytes(16));
            }
            else if (type.IsGenericType && type.GetGenericTypeDefinition() == typeof(Dictionary<,>))
            {
                var keyType = type.GetGenericArguments()[0];
                var valueType = type.GetGenericArguments()[1];
                var dictType = typeof(Dictionary<,>).MakeGenericType(keyType, valueType);
                var dict = (IDictionary)Activator.CreateInstance(dictType);
                int count = reader.ReadInt32();
                var genericDeserializeKeyMethod = thisMethod.MakeGenericMethod(keyType);
                var genericDeserializeValueMethod = thisMethod.MakeGenericMethod(valueType);
                for (int i = 0; i < count; i++)
                {
                    var key = genericDeserializeKeyMethod.Invoke(null, [reader]);
                    var value = genericDeserializeValueMethod.Invoke(null, [reader]);
                    dict.Add(key, value);
                }
                return (T)dict;
            }
            else if (type.IsGenericType && type.GetGenericTypeDefinition() == typeof(IList<>))
            {
                var elementType = type.GetGenericArguments()[0];
                var listType = typeof(List<>).MakeGenericType(elementType);
                var list = (IList)Activator.CreateInstance(listType);
                int count = reader.ReadInt32();
                var genericDeserializeMethod = thisMethod.MakeGenericMethod(elementType);
                for (int i = 0; i < count; i++)
                {
                    var item = genericDeserializeMethod.Invoke(null, [reader]);
                    list.Add(item);
                }
                return (T)list;
            }
            else if (type.IsArray)
            {
                var elementType = type.GetElementType();
                int count = reader.ReadInt32();
                var array = Array.CreateInstance(elementType, count);
                var genericDeserializeMethod = thisMethod.MakeGenericMethod(elementType);
                for (int i = 0; i < count; i++)
                {
                    var item = genericDeserializeMethod.Invoke(null, [reader]);
                    array.SetValue(item, i);
                }
                return (T)(object)array;
            }
            else if (type.GetCustomAttribute<ByteSerializableAttribute>() != null)
            {
                object instance = Activator.CreateInstance<T>(); // https://stackoverflow.com/a/27226969/2131563, box when T is struct, so that SetValue can work
                var members = type
                    .GetMembers(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance)
                    .Where(m => m.MemberType == MemberTypes.Field || m.MemberType == MemberTypes.Property)
                    .OrderBy(m => m.MetadataToken);

                foreach (var member in members)
                {
                    if (member is FieldInfo fieldInfo)
                    {
                        var genericDeserializeMethod = thisMethod.MakeGenericMethod(fieldInfo.FieldType);
                        var memberValue = genericDeserializeMethod.Invoke(null, [reader]);
                        fieldInfo.SetValue(instance, memberValue);
                    }
                    else if (member is PropertyInfo propertyInfo)
                    {
                        var genericDeserializeMethod = thisMethod.MakeGenericMethod(propertyInfo.PropertyType);
                        var memberValue = genericDeserializeMethod.Invoke(null, [reader]);
                        propertyInfo.SetValue(instance, memberValue);
                    }
                }
                return (T)instance;
            }
            else
            {
                throw new NotImplementedException($"Not implemented type {type.FullName}");
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

    public class CustomedBinaryReader : BinaryReader
    {
        protected static Encoding encoding = Encoding.UTF8;

        public CustomedBinaryReader(Stream stream)
            : base(stream, encoding)
        {
        }

        public override string ReadString()
        {
            int byteCount = ReadInt32();
            char[] chars = ReadChars(byteCount);
            return new string(chars);
        }
    }

    [AttributeUsage(AttributeTargets.Struct | AttributeTargets.Class)]
    public class ByteSerializableAttribute : Attribute
    {

    }
}
