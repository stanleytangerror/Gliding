using ModelProcess;
using System.Text.Json;

//var modelName = "2019_porsche_935_martini_racing-gltf"; // https://sketchfab.com/3d-models/hintze-hall-vr-tour-058b26fb31ed49df978de31af3dc091f
//var buildJsonName = $"D:\\Assets\\{modelName}\\build.json";
//var buildBinName = $"D:\\Assets\\{modelName}\\build.bin";

//var model = GltfLoader.Load($"D:\\Assets\\{modelName}\\scene.gltf");

var buildJsonName = $"D:\\Assets\\Bistro_v5_2\\build.json";
var buildBinName = $"D:\\Assets\\Bistro_v5_2\\build.bin";
var model = FbxLoader.Load($"D:\\Assets\\Bistro_v5_2\\3dpea.com_BistroInterior.fbx");

var storageDate = model.ToStorageData();

string jsonString = JsonSerializer.Serialize(storageDate, new JsonSerializerOptions { IncludeFields = true, WriteIndented = true });
File.WriteAllText(buildJsonName, jsonString);

using MemoryStream stream = new();
using CustomedBinaryWriter writer = new(stream);
writer.Serialize(storageDate);
var bytes = stream.ToArray();
File.WriteAllBytes(buildBinName, bytes);

using MemoryStream stream2 = new(bytes);
using CustomedBinaryReader reader = new(stream2);
var d = reader.Deserialize<ModelData>(null);
Console.WriteLine(d.Name);