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
    case DKShaderDataType::Unknown:				return "Unknown";
    case DKShaderDataType::None:				return "None";

    case DKShaderDataType::Struct:				return "Struct";
    case DKShaderDataType::Texture:				return "Texture";
    case DKShaderDataType::Sampler:				return "Sampler";

    case DKShaderDataType::Float:				return "Float";
    case DKShaderDataType::Float2:				return "Float2";
    case DKShaderDataType::Float3:				return "Float3";
    case DKShaderDataType::Float4:				return "Float4";

    case DKShaderDataType::Float2x2:			return "Float2x2";
    case DKShaderDataType::Float2x3:			return "Float2x3";
    case DKShaderDataType::Float2x4:			return "Float2x4";

    case DKShaderDataType::Float3x2:			return "Float3x2";
    case DKShaderDataType::Float3x3:			return "Float3x3";
    case DKShaderDataType::Float3x4:			return "Float3x4";

    case DKShaderDataType::Float4x2:			return "Float4x2";
    case DKShaderDataType::Float4x3:			return "Float4x3";
    case DKShaderDataType::Float4x4:			return "Float4x4";

    case DKShaderDataType::Half:				return "Half";
    case DKShaderDataType::Half2:				return "Half2";
    case DKShaderDataType::Half3:				return "Half3";
    case DKShaderDataType::Half4:				return "Half4";

    case DKShaderDataType::Half2x2:				return "Half2x2";
    case DKShaderDataType::Half2x3:				return "Half2x3";
    case DKShaderDataType::Half2x4:				return "Half2x4";

    case DKShaderDataType::Half3x2:				return "Half3x2";
    case DKShaderDataType::Half3x3:				return "Half3x3";
    case DKShaderDataType::Half3x4:				return "Half3x4";

    case DKShaderDataType::Half4x2:				return "Half4x2";
    case DKShaderDataType::Half4x3:				return "Half4x3";
    case DKShaderDataType::Half4x4:				return "Half4x4";

    case DKShaderDataType::Int:					return "Int";
    case DKShaderDataType::Int2:				return "Int2";
    case DKShaderDataType::Int3:				return "Int3";
    case DKShaderDataType::Int4:				return "Int4";

    case DKShaderDataType::UInt:				return "UInt";
    case DKShaderDataType::UInt2:				return "UInt2";
    case DKShaderDataType::UInt3:				return "UInt3";
    case DKShaderDataType::UInt4:				return "UInt4";

    case DKShaderDataType::Short:				return "Short";
    case DKShaderDataType::Short2:				return "Short2";
    case DKShaderDataType::Short3:				return "Short3";
    case DKShaderDataType::Short4:				return "Short4";

    case DKShaderDataType::UShort:				return "UShort";
    case DKShaderDataType::UShort2:				return "UShort2";
    case DKShaderDataType::UShort3:				return "UShort3";
    case DKShaderDataType::UShort4:				return "UShort4";

    case DKShaderDataType::Char:				return "Char";
    case DKShaderDataType::Char2:				return "Char2";
    case DKShaderDataType::Char3:				return "Char3";
    case DKShaderDataType::Char4:				return "Char4";

    case DKShaderDataType::UChar:				return "UChar";
    case DKShaderDataType::UChar2:				return "UChar2";
    case DKShaderDataType::UChar3:				return "UChar3";
    case DKShaderDataType::UChar4:				return "UChar4";

    case DKShaderDataType::Bool:				return "Bool";
    case DKShaderDataType::Bool2:				return "Bool2";
    case DKShaderDataType::Bool3:				return "Bool3";
    case DKShaderDataType::Bool4:				return "Bool4";
    }
    return "Error";
}

void PrintShaderResource(const DKShaderResource& res, DKLogCategory c = DKLogCategory::Info)
{
    struct MemberPrinter
    {
        const DKShaderResource& res;
        int indent;
        DKLogCategory c;
        void operator()(const DKShaderResourceStruct& str) const
        {
            DKString indentStr = "";
            for (int i = 0; i < indent; ++i)
            {
                indentStr += "    ";
            }
            for (const DKShaderResourceStructMember& mem : str.members)
            {
                if (mem.count > 1)
                {
                    DKLog(c, " %ls+ %ls[%d] (%s, Offset: %d, Stride: %d)",
                        (const wchar_t*)indentStr,
                        (const wchar_t*)mem.name,
                        mem.count,
                        ShaderDataTypeStr(mem.dataType),
                        mem.offset, mem.stride);

                }
                else
                {
                    DKLog(c, " %ls+ %ls (%s, Offset: %d)",
                        (const wchar_t*)indentStr,
                        (const wchar_t*)mem.name,
                        ShaderDataTypeStr(mem.dataType),
                        mem.offset);
                }

                auto* p = res.structTypeMemberMap.Find(mem.typeInfoKey);
                if (p)
                {
                    DKLog(c, " %ls  Struct \"%ls\"",
                        (const wchar_t*)indentStr,
                        (const wchar_t*)mem.typeInfoKey);
                    MemberPrinter{ res, indent + 1, c }.operator()(p->value);
                }
            }
        }
    };


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
    }
    else
    {
        DKLog(c, " Type:%s, Access:%s, Enabled:%d",
            type,
            access,
            int(res.enabled));
    }
    if (res.typeInfoKey.Length() > 0)
        DKLog(c, " Struct \"%ls\"", (const wchar_t*)res.typeInfoKey);
    if (res.type == DKShaderResource::TypeBuffer)
    {
        auto p = res.structTypeMemberMap.Find(res.typeInfoKey);
        if (p)
            MemberPrinter{ res, 1 , c }.operator()(p->value);
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
        DKLog(c, "  [in] ShaderAttribute[%d]: \"%ls\" (location:%u)",
            i, (const wchar_t*)attr.name, attr.location);
    }
    DKLog(c, "---------------------------------------------------------");
    DKLog(c, "Shader<%ls.SPIR-V>.OutputAttributes: %d",
        (const wchar_t*)stage, shader->OutputAttributes().Count());
    for (int i = 0; i < shader->OutputAttributes().Count(); ++i)
    {
        const DKShaderAttribute& attr = shader->OutputAttributes().Value(i);
        DKLog(c, "  [out] ShaderAttribute[%d]: \"%ls\" (location:%u)",
            i, (const wchar_t*)attr.name, attr.location);
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
        DKLog(c, "  [in] ShaderAttribute[%d]: \"%ls\" (location:%u)",
            i, (const wchar_t*)attr.name, attr.location);
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
    }
    DKLog(c, "=========================================================");
}
