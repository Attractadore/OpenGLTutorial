struct LightAttenuation {
    float minRadius;
    float maxRadius;
};

float disAtt(float dist, LightAttenuation la){
    float distFactor = la.minRadius / (max(dist, la.minRadius));
    float winFactor = max(1.0f - pow(dist / la.maxRadius, 4.0f), 0.0f);
    return pow(distFactor * winFactor, 2);
}

float angAtt(float t, float inner, float outer){
    return pow(clamp((t - outer) / (inner - outer), 0.0f, 1.0f), 2.0f);
}
