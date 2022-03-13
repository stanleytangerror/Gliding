#define GraphicsRS \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), "\
" "\
"DescriptorTable( "\
"    SRV(t0, numDescriptors = 16, space = 0), "\
"    visibility = SHADER_VISIBILITY_PIXEL "\
"    ), "\
" "\
"CBV(b0, space = 0), "\
"CBV(b1, space = 0), "\
" "\
"StaticSampler(s0, "\
"    filter = FILTER_MIN_MAG_MIP_LINEAR, "\
"    addressU = TEXTURE_ADDRESS_CLAMP, "\
"    addressV = TEXTURE_ADDRESS_CLAMP, "\
"    addressW = TEXTURE_ADDRESS_CLAMP, "\
"    mipLODBias = 0, "\
"    maxAnisotropy = 0, "\
"    comparisonFunc = COMPARISON_NEVER, "\
"    borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK, "\
"    minLOD = 0.0f, "\
"    maxLOD = 3.402823466e+38f, "\
"    space = 0, "\
"    visibility = SHADER_VISIBILITY_PIXEL "\
"    ) "\
", "\
"StaticSampler(s1, "\
"    filter = FILTER_MIN_MAG_MIP_POINT, "\
"    addressU = TEXTURE_ADDRESS_WRAP, "\
"    addressV = TEXTURE_ADDRESS_WRAP, "\
"    addressW = TEXTURE_ADDRESS_WRAP, "\
"    mipLODBias = 0, "\
"    maxAnisotropy = 0, "\
"    comparisonFunc = COMPARISON_NEVER, "\
"    borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK, "\
"    minLOD = 0.0f, "\
"    maxLOD = 3.402823466e+38f, "\
"    space = 0, "\
"    visibility = SHADER_VISIBILITY_PIXEL "\
"    ) "\
", "\
"StaticSampler(s2, "\
"    filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, "\
"    addressU = TEXTURE_ADDRESS_CLAMP, "\
"    addressV = TEXTURE_ADDRESS_CLAMP, "\
"    addressW = TEXTURE_ADDRESS_CLAMP, "\
"    mipLODBias = 0, "\
"    maxAnisotropy = 0, "\
"    comparisonFunc = COMPARISON_NEVER, "\
"    borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK, "\
"    minLOD = 0.0f, "\
"    maxLOD = 3.402823466e+38f, "\
"    space = 0, "\
"    visibility = SHADER_VISIBILITY_PIXEL "\
"    ) "\
", "\
"StaticSampler(s3, "\
"    filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, "\
"    addressU = TEXTURE_ADDRESS_CLAMP, "\
"    addressV = TEXTURE_ADDRESS_CLAMP, "\
"    addressW = TEXTURE_ADDRESS_WRAP, "\
"    mipLODBias = 0, "\
"    maxAnisotropy = 0, "\
"    comparisonFunc = COMPARISON_NEVER, "\
"    borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK, "\
"    minLOD = 0.0f, "\
"    maxLOD = 3.402823466e+38f, "\
"    space = 0, "\
"    visibility = SHADER_VISIBILITY_PIXEL "\
"    ) "

#define ComputeCopy \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), "\
" "\
"DescriptorTable( "\
"    SRV(t0, numDescriptors = 16, space = 0) "\
"    ), "\
"DescriptorTable( "\
"    UAV(u0, numDescriptors = 16, space = 0) "\
"    ) "\
" "
