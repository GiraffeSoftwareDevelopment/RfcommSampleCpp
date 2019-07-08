
#include "pch.h"

#include "MainPage.xaml.h"

using namespace RfcommSampleCpp;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::Rfcomm;
using namespace Concurrency;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

MainPage::MainPage()
{
	InitializeComponent();

	Platform::String^ example("{4E989E9F-F81A-4EE8-B9A7-BB74D5CD3C96}");
	GUID rawguid;
	HRESULT hr = IIDFromString(example->Data(), &rawguid);
	if (SUCCEEDED(hr))
	{
		m_rfcommChatServiceUuid = Platform::Guid(rawguid);
	}
	m_sdpServiceNameAttributeType = (4 << 3) | 5;
	m_sdpServiceName = L"bluetooth rfcomm sample";
	m_sdpServiceNameAttributeId = 0x100;
}

void MainPage::Button_server_start_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	InitializeRfcommServer();
}

void MainPage::Button_server_disconnect_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Disconnect();
	TextBox_log->Text += ("Disconnected.\n");
}

void MainPage::Button_send_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	SendMessage();
}

void MainPage::KeyboardKey_Pressed(Platform::Object^ sender, Windows::UI::Core::KeyEventArgs^ e)
{
	if (e->VirtualKey == Windows::System::VirtualKey::Enter)
	{
		SendMessage();
	}
}

void MainPage::InitializeRfcommServer()
{
	Button_server_start->IsEnabled = false;
	Button_server_disconnect->IsEnabled = true;

	auto providerTask = create_task(RfcommServiceProvider::CreateAsync(RfcommServiceId::FromUuid(m_rfcommChatServiceUuid)));
	providerTask.then([this](RfcommServiceProvider ^p) -> task <void>
	{
		m_rfcommProvider = p;
		m_socketListener = ref new StreamSocketListener();
		m_socketListener->ConnectionReceived += 
			ref new Windows::Foundation::TypedEventHandler< Windows::Networking::Sockets::StreamSocketListener ^, 
			Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs ^>(this, &MainPage::OnConnectionReceived);

		Platform::String ^rfcomm = m_rfcommProvider->ServiceId->AsString();
		return create_task(m_socketListener->BindServiceNameAsync(rfcomm)).then([this]()
		{
			DataWriter ^sdpWriter = ref new DataWriter();
			sdpWriter->WriteByte(m_sdpServiceNameAttributeType);
			sdpWriter->WriteByte((byte)m_sdpServiceName->Length());
			sdpWriter->UnicodeEncoding = Windows::Storage::Streams::UnicodeEncoding::Utf8;
			sdpWriter->WriteString(m_sdpServiceName);
			m_rfcommProvider->SdpRawAttributes->Insert(m_sdpServiceNameAttributeId, sdpWriter->DetachBuffer());
			m_rfcommProvider->StartAdvertising(m_socketListener, true);
		});
	}).then([this](task<void> t)
	{
		try
		{
			t.get();
			Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this]()
			{
				TextBox_log->Text += ("Listening for incoming connections\n");
			}));
		}
		catch (Platform::Exception ^ex)
		{
			OutputDebugString(ex->Message->Data());
		}
	});
}

void MainPage::OnConnectionReceived(StreamSocketListener ^sender, StreamSocketListenerConnectionReceivedEventArgs ^args)
{
	delete m_socketListener;
	m_socketListener = nullptr;
	try
	{
		m_socket = args->Socket;
	}
	catch (Exception ^e)
	{
		Platform::String ^output = L"OnConnectionReceived : Exception occured : ";
		output += e->Message;
		output += L"\n";
		OutputDebugString(output->Data());
		Disconnect();
		return;
	}
	auto hostNameTask = create_task(BluetoothDevice::FromHostNameAsync(m_socket->Information->RemoteHostName));
	hostNameTask.then([this](BluetoothDevice ^device)
	{
		m_remoteDevice = device;
		m_writer = ref new DataWriter(m_socket->OutputStream);
		m_reader = ref new DataReader(m_socket->InputStream);
		Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this]()
		{
			TextBox_log->Text += Format(L"Connected to Client : %s\n", m_remoteDevice->Name->Data());

		}));
	}).then([this]()
	{
		m_receiveString = L"";
		ReadLoop();
	});
}

void MainPage::SendMessage()
{
	if (TextBox_message->Text->Length() != 0)
	{
		if (m_socket != nullptr)
		{
			Platform::String ^message = TextBox_message->Text;
			m_writer->WriteString(message);

			Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this, message]()
			{
				TextBox_log->Text += Format(L"Sent : %s\n", message->Data());
				TextBox_message->Text = L"";
			}));
			create_task(m_writer->StoreAsync()).then([this](unsigned int bytesStored)
			{
				return m_writer->FlushAsync();
			});
		}
		else
		{
			TextBox_log->Text += L"No clients connected, please wait for a client to connect before attempting to send a message";
		}
	}
}

void MainPage::ReadLoop()
{
	try
	{
		create_task(m_reader->LoadAsync(1)).then([this](task<unsigned int> loadedSize)
		{
			try
			{
				unsigned char b = 0;
				unsigned int size = loadedSize.get();
				if (0 < size)
				{
					b = m_reader->ReadByte();
					wchar_t wcharBuffer[2] = { b };
					wcharBuffer[1] = L'\0';
					std::wstring wstr = wcharBuffer;
					Platform::String ^str = ref new Platform::String(wstr.c_str());
					m_receiveString += str;
					if ('\n' == b)
					{
						Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this]()
							{
								TextBox_log->Text += Format(L"Received : %s : %s\n", m_remoteDevice->Name->Data(), m_receiveString->Data());
								m_receiveString = L"";
							}));
					}
				}
				else
				{
				}
				ReadLoop();
			}
			catch (Exception ^ex)
			{
				OutputDebugString(ex->Message->Data());
			}
		});
	}
	catch (Exception ^ex)
	{
		OutputDebugString(ex->Message->Data());
	}
}

Platform::String ^MainPage::Format(const wchar_t *format, ...)
{
	std::va_list args;
	va_start(args, format);
	Platform::String ^result = Format(format, args);
	va_end(args);
	return(result);
}

Platform::String ^MainPage::Format(const wchar_t *format, va_list arg)
{
	#define STRING_MAX 1024
	wchar_t buffer[STRING_MAX];
	
	vswprintf_s<1024>(buffer, format, arg);
	std::wstring wstr(buffer);
	Platform::String ^result = ref new Platform::String(wstr.c_str());
	return(result);
}

void MainPage::Disconnect()
{
	if (nullptr != m_reader)
	{
		delete m_reader;
		m_reader = nullptr;
	}
	if (m_rfcommProvider != nullptr)
	{
		m_rfcommProvider->StopAdvertising();
		m_rfcommProvider = nullptr;
	}
	if (m_socketListener != nullptr)
	{
		delete m_socketListener;
		m_socketListener = nullptr;
	}
	if (m_writer != nullptr)
	{
		m_writer->DetachStream();
		m_writer = nullptr;
	}
	if (m_socket != nullptr)
	{
		delete m_socket;
		m_socket = nullptr;
	}
	{
		Button_server_start->IsEnabled = true;
		Button_server_disconnect->IsEnabled = false;
	}
}
