float model(float3 k, __constant float * param)
{


    float pi = 3.14159;

    float a = param[0] * (2.0f - native_cos(k.x * 2 * pi) * (native_cos(k.y * 2 * pi) + native_cos(k.z * 2 * pi))) + (2.0f * param[2] - param[0]) * (1.0f - native_cos(k.y * 2 * pi) * native_cos(k.z * 2 * pi));

    float b = param[0] * (2.0f - native_cos(k.y * 2 * pi) * (native_cos(k.x * 2 * pi) + native_cos(k.z * 2 * pi))) + (2.0f * param[2] - param[0]) * (1.0f - native_cos(k.x * 2 * pi) * native_cos(k.z * 2 * pi));

    float c = param[0] * (2.0f - native_cos(k.z * 2 * pi) * (native_cos(k.y * 2 * pi) + native_cos(k.x * 2 * pi))) + (2.0f * param[2] - param[0]) * (1.0f - native_cos(k.y * 2 * pi) * native_cos(k.x * 2 * pi));

    float d = (param[1] + param[2]) * native_sin(k.x * 2 * pi) * native_sin(k.y * 2 * pi);

    float e = (param[1] + param[2]) * native_sin(k.z * 2 * pi) * native_sin(k.y * 2 * pi);

    float f = (param[1] + param[2]) * native_sin(k.x * 2 * pi) * native_sin(k.z * 2 * pi);


    float3 Ak = (float3)(
                    (-k.x * e * e + f * k.y * e + d * k.z * e + b * c * k.x - c * d * k.y - b * f * k.z),
                    (-k.y * f * f + e * k.x * f + d * k.z * f - c * d * k.x + a * c * k.y - a * e * k.z),
                    (-k.z * d * d + e * k.x * d + f * k.y * d - b * f * k.x - a * e * k.y + a * b * k.z));

    return exp(-2.0f * (k.x * k.x + k.y * k.y + k.z * k.z) * param[3]) * native_divide(1.0f, (a * b * c - a * e * e - b * f * f - c * d * d + 2.0f * d * e * f)) * dot(k, Ak);
}
