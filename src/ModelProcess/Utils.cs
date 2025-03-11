using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace ModelProcess
{
    public static class VectorExtensions
    {
        private static class TemplateInitialization<T, TResult>
        {
            public static Func<T, TResult>? Truncate;
            public static Func<T, float, TResult>? Extend;
        }

        static VectorExtensions()
        {
            // http://blog.zhaojie.me/2014/05/zrx-quiz-1-answer.html
            TemplateInitialization<Vector4, Vector4>.Truncate = v => v;
            TemplateInitialization<Vector4, Vector3>.Truncate = v => new(v.X, v.Y, v.Z);
            TemplateInitialization<Vector4, Vector2>.Truncate = v => new(v.X, v.Y);
            TemplateInitialization<Vector3, Vector3>.Truncate = v => v;
            TemplateInitialization<Vector3, Vector2>.Truncate = v => new(v.X, v.Y);
            TemplateInitialization<Vector2, Vector2>.Truncate = v => v;
            TemplateInitialization<Vector2, Vector4>.Extend = (v, f) => new(v, f, f);
            TemplateInitialization<Vector2, Vector3>.Extend = (v, f) => new(v, f);
            TemplateInitialization<Vector2, Vector2>.Extend = (v, f) => v;
            TemplateInitialization<Vector3, Vector4>.Extend = (v, f) => new(v, f);
            TemplateInitialization<Vector3, Vector3>.Extend = (v, f) => v;
            TemplateInitialization<Vector2, Vector2>.Extend = (v, f) => v;
        }

        public static TResult Truncate<TResult>(this Vector2 v) => TemplateInitialization<Vector2, TResult>.Truncate!(v);
        public static TResult Truncate<TResult>(this Vector3 v) => TemplateInitialization<Vector3, TResult>.Truncate!(v);
        public static TResult Truncate<TResult>(this Vector4 v) => TemplateInitialization<Vector4, TResult>.Truncate!(v);

        public static TResult Extend<TResult>(this Vector2 v, float f = 0) => TemplateInitialization<Vector2, TResult>.Extend!(v, f);
        public static TResult Extend<TResult>(this Vector3 v, float f = 0) => TemplateInitialization<Vector3, TResult>.Extend!(v, f);
        public static TResult Extend<TResult>(this Vector4 v, float f = 0) => TemplateInitialization<Vector4, TResult>.Extend!(v, f);
    }

    public static class MatrixExtensions
    {
        public static string ToMathString(this Matrix4x4 matrix) =>
           $"{{ {matrix.M11}, {matrix.M12}, {matrix.M13}, {matrix.M14},\n" +
           $"  {matrix.M21}, {matrix.M22}, {matrix.M23}, {matrix.M24},\n" +
           $"  {matrix.M31}, {matrix.M32}, {matrix.M33}, {matrix.M34},\n" +
           $"  {matrix.M41}, {matrix.M42}, {matrix.M43}, {matrix.M44} }}";
    }

    public static class MathUtils
    {
        public static Matrix4x4 CreateTransform(Vector3 translation, Vector3 rotation, Vector3 scaling)
        {
            // Create scaling matrix
            var scaleMatrix = Matrix4x4.CreateScale(scaling);

            // Create rotation matrices for each axis
            var rotationXMatrix = Matrix4x4.CreateRotationX(rotation.X);
            var rotationYMatrix = Matrix4x4.CreateRotationY(rotation.Y);
            var rotationZMatrix = Matrix4x4.CreateRotationZ(rotation.Z);

            // Combine the rotation matrices
            var rotationMatrix = rotationXMatrix * rotationYMatrix * rotationZMatrix;

            // Create translation matrix
            var translationMatrix = Matrix4x4.CreateTranslation(translation);

            // Combine all transformations: scaling, rotation, and translation
            var transformMatrix = scaleMatrix * rotationMatrix * translationMatrix;

            return transformMatrix;
        }
    }
}
