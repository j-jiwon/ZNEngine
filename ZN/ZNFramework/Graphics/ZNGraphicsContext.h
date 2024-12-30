#pragma once

namespace ZNFramework
{
    class ZNGraphicsDevice;
    class ZNCommandQueue;
    class ZNRootSignature;
    class ZNConstantBuffer;
    class ZNTableDescriptorHeap;

    class GraphicsContext
    {
    public:
        static GraphicsContext& GetInstance()
        {
            static GraphicsContext instance;
            return instance;
        }

        template <typename T>
        T* GetAs() const
        {
            if constexpr (std::is_base_of_v<ZNGraphicsDevice, T>)
            {
                return dynamic_cast<T*>(device);
            }
            else if constexpr (std::is_base_of_v<ZNCommandQueue, T>)
            {
                return dynamic_cast<T*>(queue);
            }
            else if constexpr (std::is_base_of_v<ZNRootSignature, T>)
            {
                return dynamic_cast<T*>(rootSignature);
            }
            else if constexpr (std::is_base_of_v<ZNConstantBuffer, T>)
            {
                return dynamic_cast<T*>(constantBuffer);
            }
            else if constexpr (std::is_base_of_v<ZNTableDescriptorHeap, T>)
            {
                return dynamic_cast<T*>(descHeap);
            }
            else
            {
                static_assert(std::is_same_v<T, void>, "Unsupported type for GetAs");
                return nullptr;
            }
        }

        // GraphicsDevice
        void SetDevice(ZNGraphicsDevice* inDevice) { device = inDevice; }
        ZNGraphicsDevice* GetDevice() const { return device; }

        // CommandQueue
        void SetCommandQueue(ZNCommandQueue* inQueue) { queue = inQueue; }
        ZNCommandQueue* GetCommandQueue() const { return queue; }

        // RootSignature
        void SetRootSignature(ZNRootSignature* inSignature) { rootSignature = inSignature; }
        ZNRootSignature* GetRootSignature() const { return rootSignature; }

        // ConstantBuffer
        void SetConstantBuffer(ZNConstantBuffer* inConstantBuffer) { constantBuffer = inConstantBuffer; }
        ZNConstantBuffer* GetConstantBuffer() const { return constantBuffer; }

        // TableDescriptorHeap
        void SetTableDescriptorHeap(ZNTableDescriptorHeap* inDescHeap) { descHeap = inDescHeap; }
        ZNTableDescriptorHeap* GetTableDescriptorHeap() const { return descHeap; }

    private:
        ZNGraphicsDevice* device = nullptr;
        ZNCommandQueue* queue = nullptr;
        ZNRootSignature* rootSignature = nullptr;
        ZNConstantBuffer* constantBuffer = nullptr;
        ZNTableDescriptorHeap* descHeap = nullptr;

        GraphicsContext() = default;
        ~GraphicsContext() = default;
        GraphicsContext(const GraphicsContext&) = delete;
        GraphicsContext& operator=(const GraphicsContext&) = delete;
    };
}
