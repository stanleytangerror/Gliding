using ModelProcess;
using System.Text.Json;

var model = GltfLoader.Load("D:\\Assets\\free_1975_porsche_911_930_turbo\\scene.gltf");

string jsonString = JsonSerializer.Serialize(model, new JsonSerializerOptions { IncludeFields = true });
Console.WriteLine(jsonString);
