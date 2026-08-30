// Render Toy default scene pass. Camera controls are supplied by Workbench.
struct VSOut
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

[shader("vertex")]
VSOut vsMain(uint vertexID : SV_VertexID)
{
    float2 p = float2((vertexID << 1) & 2, vertexID & 2);
    VSOut o;
    o.uv = p;
    o.position = float4(p * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}

float sdSphere(float3 p, float r) { return length(p) - r; }
float sdBox(float3 p, float3 b) { float3 q = abs(p) - b; return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0); }
float sdTorus(float3 p, float2 t) { float2 q = float2(length(p.xz) - t.x, p.y); return length(q) - t.y; }

// Nearest surface as (distance, materialId): a few primitives on a ground plane.
float2 mapScene(float3 p)
{
    float2 res = float2(p.y + 1.0, 0.0);
    float2 sph = float2(sdSphere(p - float3(-1.35, -0.10, 0.0), 0.90), 1.0);
    if (sph.x < res.x) res = sph;
    float2 box = float2(sdBox(p - float3(1.25, -0.35, 0.25), float3(0.55, 0.65, 0.55)), 2.0);
    if (box.x < res.x) res = box;
    float2 tor = float2(sdTorus(p - float3(0.05, 0.05, -1.55), float2(0.55, 0.20)), 3.0);
    if (tor.x < res.x) res = tor;
    return res;
}

float3 sceneNormal(float3 p)
{
    float2 e = float2(0.0015, 0.0);
    return normalize(float3(
        mapScene(p + e.xyy).x - mapScene(p - e.xyy).x,
        mapScene(p + e.yxy).x - mapScene(p - e.yxy).x,
        mapScene(p + e.yyx).x - mapScene(p - e.yyx).x));
}

float softShadow(float3 ro, float3 rd, float mint, float maxt)
{
    float res = 1.0;
    float t = mint;
    for (int i = 0; i < 48; ++i)
    {
        float h = mapScene(ro + rd * t).x;
        res = min(res, 10.0 * h / t);
        t += clamp(h, 0.02, 0.30);
        if (h < 0.001 || t > maxt) break;
    }
    return saturate(res);
}

float calcAO(float3 p, float3 n)
{
    float occ = 0.0;
    float sca = 1.0;
    for (int i = 0; i < 5; ++i)
    {
        float hr = 0.01 + 0.12 * float(i) / 4.0;
        float d = mapScene(p + n * hr).x;
        occ += (hr - d) * sca;
        sca *= 0.95;
    }
    return saturate(1.0 - 3.0 * occ);
}

float3 materialColor(float id, float3 p)
{
    if (id < 0.5)
    {
        float checker = fmod(floor(p.x) + floor(p.z), 2.0);
        return lerp(float3(0.19, 0.20, 0.23), float3(0.30, 0.31, 0.35), checker);
    }
    if (id < 1.5) return float3(0.90, 0.42, 0.24); // sphere
    if (id < 2.5) return float3(0.26, 0.55, 0.74); // box
    return float3(0.64, 0.42, 0.80);               // torus
}

// Real viewport aspect from screen-space UV derivatives (no resolution uniform).
float viewAspect(float2 uv) { return abs(ddy(uv.y)) / max(abs(ddx(uv.x)), 1.0e-8); }

float3 renderScene(float2 uv, float aspect)
{
    float2 ndc = uv * 2.0 - 1.0;
    ndc.y = -ndc.y;
    ndc.x *= aspect;

    float yaw = camera.camYaw + 0.7;
    float pitch = clamp(camera.camPitch + 0.35, -1.45, 1.45);
    float dist = clamp(camera.camDistance + 5.0, 1.6, 40.0);
    float fov = clamp(camera.camFov + 45.0, 5.0, 120.0) * 0.017453292;

    float cy = cos(yaw), sy = sin(yaw);
    float cp = cos(pitch), sp = sin(pitch);
    float3 target = float3(camera.camPanX, camera.camPanY, 0.0);
    float3 ro = target + float3(sy * cp, sp, cy * cp) * dist;
    float3 fwd = normalize(target - ro);
    float3 right = normalize(cross(float3(0.0, 1.0, 0.0), fwd));
    float3 up = cross(fwd, right);
    float t = tan(fov * 0.5);
    float3 rd = normalize(fwd + right * ndc.x * t + up * ndc.y * t);

    float tt = 0.0;
    float id = -1.0;
    float3 pos = ro;
    for (int i = 0; i < 128; ++i)
    {
        pos = ro + rd * tt;
        float2 h = mapScene(pos);
        if (h.x < 0.001) { id = h.y; break; }
        tt += h.x;
        if (tt > 60.0) break;
    }

    float3 sky = lerp(float3(0.05, 0.07, 0.11), float3(0.13, 0.19, 0.30), saturate(rd.y * 0.5 + 0.5));
    if (id < 0.0) return sky;

    float3 n = sceneNormal(pos);
    float3 base = materialColor(id, pos);

    float3 lightDir = normalize(float3(0.6, 0.75, 0.35));
    float diff = saturate(dot(n, lightDir));
    float sh = softShadow(pos + n * 0.02, lightDir, 0.02, 20.0);
    float ao = calcAO(pos, n);

    float3 skyAmb = lerp(float3(0.10, 0.12, 0.17), float3(0.28, 0.33, 0.42), saturate(n.y * 0.5 + 0.5));
    float3 col = base * (skyAmb * ao + diff * sh * float3(1.0, 0.95, 0.85) * 1.6);

    float fres = pow(1.0 - saturate(dot(n, -rd)), 4.0);
    col += fres * 0.12 * ao;

    float fog = saturate(tt / 60.0);
    col = lerp(col, sky, fog * 0.30);
    return col;
}

[shader("fragment")]
SceneSample psMain(VSOut input)
{
    float3 col = renderScene(input.uv, viewAspect(input.uv));
    col = col / (col + 1.0);                       // Reinhard tone map
    col = pow(col, float3(0.4545, 0.4545, 0.4545)); // gamma
    SceneSample output;
    output.color = float4(col, 1.0);
    return output;
}
