#ifdef _WIN32
#include "Win32/stdafx.h"
#endif
#include <DK.h>

DKString ShaderStageNames(uint32_t s)
{
    DKArray<const char*> stages;
    if (s & (uint32_t)DKShaderStage::Vertex)
        stages.Add("Vertex");
    if (s & (uint32_t)DKShaderStage::TessellationControl)
        stages.Add("TessCtrl");
    if (s & (uint32_t)DKShaderStage::TessellationEvaluation)
        stages.Add("TessEval");
    if (s & (uint32_t)DKShaderStage::Geometry)
        stages.Add("Geometry");
    if (s & (uint32_t)DKShaderStage::Fragment)
        stages.Add("Fragment");
    if (s & (uint32_t)DKShaderStage::Compute)
        stages.Add("Compute");

    if (stages.IsEmpty())
        return "";

    DKString str = stages.Value(0);
    for (int i = 1; i < stages.Count(); ++i)
        str += DKString::Format(", %ls", stages.Value(i));
    return str;
}

const char* ShaderDataTypeStr(DKShaderDataType t)
{
    switch (t) {
    case DKShaderDataType::Unknown:			return "Unknown";
    case DKShaderDataType::None:			return "None";

    case DKShaderDataType::Struct:			return "Struct";
    case DKShaderDataType::Texture:			return "Texture";
    case DKShaderDataType::Sampler:			return "Sampler";

    case DKShaderDataType::Bool:            return "Bool";
    case DKShaderDataType::BoolV2:          return "BoolV2";
    case DKShaderDataType::BoolV3:          return "BoolV3";
    case DKShaderDataType::BoolV4:          return "BoolV4";

    case DKShaderDataType::Int8:            return "Int8";
    case DKShaderDataType::Int8V2:          return "Int8V2";
    case DKShaderDataType::Int8V3:          return "Int8V3";
    case DKShaderDataType::Int8V4:          return "Int8V4";

    case DKShaderDataType::UInt8:           return "UInt8";
    case DKShaderDataType::UInt8V2:         return "UInt8V2";
    case DKShaderDataType::UInt8V3:         return "UInt8V3";
    case DKShaderDataType::UInt8V4:         return "UInt8V4";

    case DKShaderDataType::Int16:           return "Int16";
    case DKShaderDataType::Int16V2:         return "Int16V2";
    case DKShaderDataType::Int16V3:         return "Int16V3";
    case DKShaderDataType::Int16V4:         return "Int16V4";

    case DKShaderDataType::UInt16:          return "UInt16";
    case DKShaderDataType::UInt16V2:        return "UInt16V2";
    case DKShaderDataType::UInt16V3:        return "UInt16V3";
    case DKShaderDataType::UInt16V4:        return "UInt16V4";

    case DKShaderDataType::Int32:           return "Int32";
    case DKShaderDataType::Int32V2:         return "Int32V2";
    case DKShaderDataType::Int32V3:         return "Int32V3";
    case DKShaderDataType::Int32V4:         return "Int32V4";

    case DKShaderDataType::UInt32:          return "UInt32";
    case DKShaderDataType::UInt32V2:        return "UInt32V2";
    case DKShaderDataType::UInt32V3:        return "UInt32V3";
    case DKShaderDataType::UInt32V4:        return "UInt32V4";

    case DKShaderDataType::Int64:           return "Int64";
    case DKShaderDataType::Int64V2:         return "Int64V2";
    case DKShaderDataType::Int64V3:         return "Int64V3";
    case DKShaderDataType::Int64V4:         return "Int64V4";

    case DKShaderDataType::UInt64:          return "UInt64";
    case DKShaderDataType::UInt64V2:        return "UInt64V2";
    case DKShaderDataType::UInt64V3:        return "UInt64V3";
    case DKShaderDataType::UInt64V4:        return "UInt64V4";

    case DKShaderDataType::Float16:         return "Float16";
    case DKShaderDataType::Float16V2:       return "Float16V2";
    case DKShaderDataType::Float16V3:       return "Float16V3";
    case DKShaderDataType::Float16V4:       return "Float16V4";
    case DKShaderDataType::Float16M2x2:     return "Float16M2x2";
    case DKShaderDataType::Float16M3x2:     return "Float16M3x2";
    case DKShaderDataType::Float16M4x2:     return "Float16M4x2";
    case DKShaderDataType::Float16M2x3:     return "Float16M2x3";
    case DKShaderDataType::Float16M3x3:     return "Float16M3x3";
    case DKShaderDataType::Float16M4x3:     return "Float16M4x3";
    case DKShaderDataType::Float16M2x4:     return "Float16M2x4";
    case DKShaderDataType::Float16M3x4:     return "Float16M3x4";
    case DKShaderDataType::Float16M4x4:     return "Float16M4x4";

    case DKShaderDataType::Float32:         return "Float32";
    case DKShaderDataType::Float32V2:       return "Float32V2";
    case DKShaderDataType::Float32V3:       return "Float32V3";
    case DKShaderDataType::Float32V4:       return "Float32V4";
    case DKShaderDataType::Float32M2x2:     return "Float32M2x2";
    case DKShaderDataType::Float32M3x2:     return "Float32M3x2";
    case DKShaderDataType::Float32M4x2:     return "Float32M4x2";
    case DKShaderDataType::Float32M2x3:     return "Float32M2x3";
    case DKShaderDataType::Float32M3x3:     return "Float32M3x3";
    case DKShaderDataType::Float32M4x3:     return "Float32M4x3";
    case DKShaderDataType::Float32M2x4:     return "Float32M2x4";
    case DKShaderDataType::Float32M3x4:     return "Float32M3x4";
    case DKShaderDataType::Float32M4x4:     return "Float32M4x4";

    case DKShaderDataType::Float64:         return "Float64";
    case DKShaderDataType::Float64V2:       return "Float64V2";
    case DKShaderDataType::Float64V3:       return "Float64V3";
    case DKShaderDataType::Float64V4:       return "Float64V4";
    case DKShaderDataType::Float64M2x2:     return "Float64M2x2";
    case DKShaderDataType::Float64M3x2:     return "Float64M3x2";
    case DKShaderDataType::Float64M4x2:     return "Float64M4x2";
    case DKShaderDataType::Float64M2x3:     return "Float64M2x3";
    case DKShaderDataType::Float64M3x3:     return "Float64M3x3";
    case DKShaderDataType::Float64M4x3:     return "Float64M4x3";
    case DKShaderDataType::Float64M2x4:     return "Float64M2x4";
    case DKShaderDataType::Float64M3x4:     return "Float64M3x4";
    case DKShaderDataType::Float64M4x4:     return "Float64M4x4";
    }
    return "Error";
}

void PrintShaderResourceStructMember(const DKShaderResourceStructMember& member,
                                     const DKString prefix,
                                     int indent,
                                     DKLogCategory c = DKLogCategory::Info)
{
    DKString indentStr = "";
    for (int i = 0; i < indent; ++i)
    {
        indentStr += "    ";
    }

    if (member.count > 1)
    {
        DKLog(c, "%ls %ls+ %ls[%d] (%s, Offset: %d, size: %d, stride: %d)",
              (const wchar_t*)prefix,
              (const wchar_t*)indentStr,
              (const wchar_t*)member.name,
              member.count,
              ShaderDataTypeStr(member.dataType),
              member.offset, member.size, member.stride);
    }
    else
    {
        DKLog(c, "%ls %ls+ %ls (%s, Offset: %d, size: %d)",
              (const wchar_t*)prefix,
              (const wchar_t*)indentStr,
              (const wchar_t*)member.name,
              ShaderDataTypeStr(member.dataType),
              member.offset, member.size);
    }
    for (const DKShaderResourceStructMember& mem : member.members)
    {
        PrintShaderResourceStructMember(mem, prefix, indent + 1, c);
    }
}

void PrintShaderResource(const DKShaderResource& res, DKLogCategory c = DKLogCategory::Info)
{
    if (res.count > 1)
        DKLog(c, "ShaderResource: %ls[%d] (set=%d, binding=%d, stages=%ls)",
        (const wchar_t*)res.name, res.count, res.set, res.binding,
            (const wchar_t*)ShaderStageNames(res.stages));
    else
        DKLog(c, "ShaderResource: %ls (set=%d, binding=%d, stages=%ls)",
        (const wchar_t*)res.name, res.set, res.binding,
            (const wchar_t*)ShaderStageNames(res.stages));

    const char* type = "Unknown (ERROR)";
    switch (res.type)
    {
    case DKShaderResource::TypeBuffer: type = "Buffer"; break;
    case DKShaderResource::TypeTexture:	type = "Texture"; break;
    case DKShaderResource::TypeSampler:	type = "Sampler"; break;
    case DKShaderResource::TypeTextureSampler: type = "SampledTexture"; break;
    }
    const char* access = "Unknown (ERROR)";
    switch (res.access)
    {
    case DKShaderResource::AccessReadOnly:	access = "ReadOnly"; break;
    case DKShaderResource::AccessWriteOnly:	access = "WriteOnly"; break;
    case DKShaderResource::AccessReadWrite:	access = "ReadWrite"; break;
    }

    if (res.type == DKShaderResource::TypeBuffer)
    {
        DKLog(c, " Type:%s, Access:%s, Enabled:%d, Size:%d",
            type,
            access,
            int(res.enabled),
            res.typeInfo.buffer.size);

        if (res.typeInfo.buffer.dataType == DKShaderDataType::Struct)
        {
            DKLog(c, " Struct \"%ls\"", (const wchar_t*)res.name);
            for (const DKShaderResourceStructMember& mem : res.members)
            {
                PrintShaderResourceStructMember(mem, "", 1, c);
            }
        }
    }
    else
    {
        DKLog(c, " Type:%s, Access:%s, Enabled:%d",
            type,
            access,
            int(res.enabled));
    }
}

void PrintShaderReflection(const DKShader* shader, DKLogCategory c = DKLogCategory::Warning)
{
    DKString stage = ShaderStageNames((uint32_t)shader->Stage());
    if (stage.Length() == 0)
        stage = "Unknown";

    DKLog(c, "=========================================================");
    DKLog(c, "Shader<%ls.SPIR-V>.InputAttributes: %d",
        (const wchar_t*)stage, shader->InputAttributes().Count());
    for (int i = 0; i < shader->InputAttributes().Count(); ++i)
    {
        const DKShaderAttribute& attr = shader->InputAttributes().Value(i);
        DKLog(c, "  [in] ShaderAttribute[%d]: \"%ls\" (type:%s, location:%u)",
            i, (const wchar_t*)attr.name, ShaderDataTypeStr(attr.type), attr.location);
    }
    DKLog(c, "---------------------------------------------------------");
    DKLog(c, "Shader<%ls.SPIR-V>.OutputAttributes: %d",
        (const wchar_t*)stage, shader->OutputAttributes().Count());
    for (int i = 0; i < shader->OutputAttributes().Count(); ++i)
    {
        const DKShaderAttribute& attr = shader->OutputAttributes().Value(i);
        DKLog(c, "  [out] ShaderAttribute[%d]: \"%ls\" (type:%s, location:%u)",
            i, (const wchar_t*)attr.name, ShaderDataTypeStr(attr.type), attr.location);
    }
    DKLog(c, "---------------------------------------------------------");
    DKLog(c, "Shader<%ls.SPIR-V>.Resources: %d",
        (const wchar_t*)stage, shader->Resources().Count());
    for (auto& arg : shader->Resources())
        PrintShaderResource(arg, c);
    for (int i = 0; i < shader->PushConstantBufferLayouts().Count(); ++i)
    {
        const DKShaderPushConstantLayout& layout = shader->PushConstantBufferLayouts().Value(i);
        DKLog(c, " PushConstant:%d \"%ls\" (offset:%u, size:%u, stages:%ls)",
            i, (const wchar_t*)layout.name, layout.offset, layout.size,
            (const wchar_t*)ShaderStageNames(layout.stages));
        for (auto& member : layout.members)
            PrintShaderResourceStructMember(member, "", 1, c);
    }
    DKLog(c, "=========================================================");
}

void PrintPipelineReflection(const DKPipelineReflection* reflection, DKLogCategory c = DKLogCategory::Error)
{
    DKLog(c, "=========================================================");
    DKLog(c, "PipelineReflection.InputAttributes: %d", reflection->inputAttributes.Count());
    for (int i = 0; i < reflection->inputAttributes.Count(); ++i)
    {
        const DKShaderAttribute& attr = reflection->inputAttributes.Value(i);
        DKLog(c, "  [in] ShaderAttribute[%d]: \"%ls\" (type:%s, location:%u)",
            i, (const wchar_t*)attr.name, ShaderDataTypeStr(attr.type), attr.location);
    }
    DKLog(c, "---------------------------------------------------------");
    DKLog(c, "PipelineReflection.Resources: %d", reflection->resources.Count());
    for (auto& arg : reflection->resources)
        PrintShaderResource(arg, c);
    for (int i = 0; i < reflection->pushConstantLayouts.Count(); ++i)
    {
        const DKShaderPushConstantLayout& layout = reflection->pushConstantLayouts.Value(i);
        DKLog(c, " PushConstant:%d \"%ls\" (offset:%u, size:%u, stages:%ls)",
            i, (const wchar_t*)layout.name, layout.offset, layout.size,
            (const wchar_t*)ShaderStageNames(layout.stages));
        for (auto& member : layout.members)
            PrintShaderResourceStructMember(member, "", 1, c);
    }
    DKLog(c, "=========================================================");
}

class GPUGeometry
{
protected:
    DKObject<DKGpuBuffer> indexBuffer;
    DKObject<DKGpuBuffer> vertexBuffer;
    DKVertexDescriptor vertexDesc;
public:
    virtual void InitializeGpuResource(DKCommandQueue* queue) = 0;
    virtual DKGpuBuffer* VertexBuffer()  final  { return vertexBuffer; }
    virtual DKGpuBuffer* IndexBuffer()  final { return indexBuffer; }
    virtual const DKVertexDescriptor& VertexDescriptor() const final { return vertexDesc; }
};