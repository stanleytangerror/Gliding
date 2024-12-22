using ModelProcess;
using System.Text.Json;

var modelName = "hintze-hall_-_vr_tour"; // https://sketchfab.com/3d-models/hintze-hall-vr-tour-058b26fb31ed49df978de31af3dc091f

var model = GltfLoader.Load($"D:\\Assets\\{modelName}\\scene.gltf");

var storageDate = model.ToStorageData();

string jsonString = JsonSerializer.Serialize(storageDate, new JsonSerializerOptions { IncludeFields = true, WriteIndented = true });
File.WriteAllText($"D:\\Assets\\{modelName}\\build.json", jsonString);

using MemoryStream stream = new();
using CustomedBinaryWriter writer = new(stream);
writer.Serialize(storageDate);
var bytes = stream.ToArray();
File.WriteAllBytes($"D:\\Assets\\{modelName}\\build.bin", bytes);

using MemoryStream stream2 = new(bytes);
using CustomedBinaryReader reader = new(stream2);
var d = reader.Deserialize<ModelData>(null);
 Console.WriteLine(d.Name);