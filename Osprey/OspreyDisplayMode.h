#pragma once

namespace Osprey
{
    class DisplayMode : public RhRdk::Realtime::DisplayMode
    {
	public:
		DisplayMode(const CRhinoDisplayPipeline& pipeline);
		~DisplayMode() override;

		static const GUID id;

		const UUID& ClassId() const override;

		bool StartRenderer(
			const ON_2iSize& frameSize,
			const CRhinoDoc&,
			const ON_3dmView&,
			const ON_Viewport&,
			const RhRdk::Realtime::DisplayMode*) override;

		bool OnRenderSizeChanged(const ON_2iSize& newFrameSize) override;
		void ShutdownRenderer() override;

		bool RendererIsAvailable() const override;

		void CreateWorld(const CRhinoDoc& doc, const ON_3dmView& view, const CDisplayPipelineAttributes& attributes) override;

		int LastRenderedPass() const override;

		bool ShowCaptureProgress() const override;
		double Progress() const override;

		bool IsRendererStarted() const override;

		bool IsCompleted() const override;

		bool IsFrameBufferAvailable(const ON_3dmView& vp) const override;

		bool DrawOrLockRendererFrameBuffer(const FRAME_BUFFER_INFO_INPUTS& input, FRAME_BUFFER_INFO_OUTPUTS& outputs) override;
		void UnlockRendererFrameBuffer() override;

		bool UseFastDraw() override;

	private:
    };

} // namespace Osprey
