#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D image;
uniform float threshold;

void main()
{             
    vec3 color = texture(image, TexCoords).rgb;
    
    // 計算亮度
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    // 只保留超過閾值的亮度部分
    if(brightness > threshold)
        FragColor = vec4(color, 1.0);
    else
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
