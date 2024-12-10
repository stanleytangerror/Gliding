using ModelProcess;
using System.Text.Json;

var model = GltfLoader.Load("D:\\Assets\\free_1975_porsche_911_930_turbo\\scene.gltf");

string jsonString = JsonSerializer.Serialize(model, new JsonSerializerOptions { IncludeFields = true });
//Console.WriteLine(jsonString);

using MemoryStream stream = new();
using CustomedBinaryWriter writer = new(stream);

var storageDate = model.ToStorageData();
writer.Serialize(storageDate);
Console.WriteLine(stream.Length);

var bytes = stream.ToArray();
File.WriteAllBytes("D:\\Assets\\free_1975_porsche_911_930_turbo\\build.bin", bytes);

using MemoryStream stream2 = new(bytes);
using CustomedBinaryReader reader = new(stream2);
var d = reader.Deserialize<ModelData>();
Console.WriteLine(d.Name);
