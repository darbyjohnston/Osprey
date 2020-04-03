#include "stdafx.h"
#include "OspreyDisplayMode.h"

namespace Osprey
{
    const GUID DisplayMode::id =
    {
        0x057ca451,
        0x20a1,
        0x41cb,
        { 0xab, 0x88, 0x66, 0x19, 0xe1, 0xe0, 0x25, 0x68 }
    };

	const UUID& DisplayMode::ClassId() const
	{
		return id;
	}

	bool DisplayMode::StartRenderer(
		const ON_2iSize& onSize,
		const CRhinoDoc& rhinoDoc,
		const ON_3dmView& onView,
		const ON_Viewport& onVp,
		const RhRdk::Realtime::DisplayMode* pParent)
	{
		return false;
	}

	bool DisplayMode::OnRenderSizeChanged(const ON_2iSize& newSize)
	{
		return false;
	}

	void DisplayMode::ShutdownRenderer()
	{

	}

	bool DisplayMode::RendererIsAvailable() const
	{
		return false;
	}

	void DisplayMode::CreateWorld(
		const CRhinoDoc& rhinoDoc,
		const ON_3dmView& onView,
		const CDisplayPipelineAttributes& attributes)
	{

	}

	int DisplayMode::LastRenderedPass() const
	{
		return 0;
	}

	bool DisplayMode::ShowCaptureProgress() const
	{
		return false;
	}

	double DisplayMode::Progress() const
	{
		return 0.0;
	}

	bool DisplayMode::IsRendererStarted() const
	{
		return false;
	}

	bool DisplayMode::IsCompleted() const
	{
		return false;
	}

	bool DisplayMode::IsFrameBufferAvailable(const ON_3dmView& vp) const
	{
		return false;
	}

	bool DisplayMode::DrawOrLockRendererFrameBuffer(
		const FRAME_BUFFER_INFO_INPUTS& input,
		FRAME_BUFFER_INFO_OUTPUTS& outputs)
	{
		return false;
	}

	void DisplayMode::UnlockRendererFrameBuffer()
	{

	}

	bool DisplayMode::UseFastDraw()
	{
		return false;
	}

} // namespace Osprey
