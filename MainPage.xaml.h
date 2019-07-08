
#pragma once

#include <cstdarg>

#include "MainPage.g.h"

namespace RfcommSampleCpp
{
	public ref class MainPage sealed
	{
	public:
		MainPage();

	private:
		void Button_server_start_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Button_server_disconnect_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Button_send_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void KeyboardKey_Pressed(Platform::Object^ sender, Windows::UI::Core::KeyEventArgs^ e);

		void InitializeRfcommServer();
		void OnConnectionReceived(Windows::Networking::Sockets::StreamSocketListener ^sender, Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs ^args);
		void MainPage::Disconnect();

		void SendMessage();
		void ReadLoop();

	private:
		Windows::Networking::Sockets::StreamSocket ^m_socket;
		Windows::Storage::Streams::DataWriter ^m_writer;
		Windows::Storage::Streams::DataReader ^m_reader;
		Platform::String ^m_receiveString;
		Windows::Devices::Bluetooth::Rfcomm::RfcommServiceProvider ^m_rfcommProvider;
		Windows::Devices::Bluetooth::BluetoothDevice ^m_remoteDevice;
		Windows::Networking::Sockets::StreamSocketListener ^m_socketListener;
		Platform::Guid m_rfcommChatServiceUuid;
		byte m_sdpServiceNameAttributeType;
		Platform::String ^m_sdpServiceName;
		uint16 m_sdpServiceNameAttributeId;

		Platform::String ^MainPage::Format(const wchar_t *format, ...);
		Platform::String ^Format(const wchar_t *format, std::va_list arg);
	};
}
