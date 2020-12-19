using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO.Ports;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Threading;

namespace DataAnalizer
{
    public class MainViewModel : INotifyPropertyChanged
	{
		private ICommand _ConnectCommand;
		private ICommand _DisconnectCommand;
		private ICommand _RefreshCommand;
		private ICommand _ExportCommand;
		private ICommand _ClearCommand;
		private ICommand _StopCommand;

		public event EventHandler<string> DataReady;

		public MainViewModel()
		{
			RefreshCommand.Execute(null);
		}

		private SerialPort _SerialPort;

		private List<string> _Ports = new List<string>();
		public List<string> Ports
		{
			get
			{
				return _Ports;
			}
			set
            {
				_Ports = value;
				NotifyPropertyChanged();
            }
		}

		private string _PortName;
		public string PortName
		{
			get
			{
				return _PortName;
			}
			set
			{
				_PortName = value;
				NotifyPropertyChanged();
			}
		}

		private int _Throttle;
		public int Throttle
		{
			get
			{
				return _Throttle;
			}
			set
			{
				if (_Throttle != value)
				{
					_Throttle = value;
					UpdateThrottlePosition();

					NotifyPropertyChanged();
				}
			}
		}

		private bool _Connected;
		public bool Connected
		{
			get
			{
				return _Connected;
			}
			set
			{
				_Connected = value;
				NotifyPropertyChanged();
			}
		}

		private int _PolesCount = 14;
		public int PolesCount
		{
			get
			{
				return _PolesCount;
			}
			set
			{
				if (_PolesCount != value)
				{
					_PolesCount = value;
					NotifyPropertyChanged();
				}
			}
		}

		public void ClearHistory()
		{
			History.Clear();
		}

		private ObservableCollection<DataPacket> _History = new ObservableCollection<DataPacket>();

		public ObservableCollection<DataPacket> History
		{
			get
			{
				return _History;
			}
			set
			{
				_History = value;
				NotifyPropertyChanged();
			}
		}

		public void ReadCurrentPort()
		{
			while (_SerialPort?.IsOpen == true)
			{
				try
				{
					string message = _SerialPort.ReadLine();
					if (!string.IsNullOrWhiteSpace(message))
					{
						var packet = new DataPacket(message);
						if (packet.NotEmpty())
						{
							Application.Current.Dispatcher.BeginInvoke(DispatcherPriority.DataBind, new Action(() =>
							{
								History.Add(packet);
							}));
						}
					}
					
				}
				catch (TimeoutException) { }
				catch (Exception) { }
			}
		}

		private void UpdateThrottlePosition()
		{
			try
			{
				if (_SerialPort?.IsOpen == true)
				{
					_SerialPort.Write(new byte[] { (byte)Throttle}, 0, 1);
				}
			}
			catch (TimeoutException)
			{ }
		}
		
		public ICommand StopCommand
		{
			get
			{
				_StopCommand = _StopCommand ?? new DelegateCommand(() =>
				{
					Throttle = 0;
				}, () =>
				{
					return _SerialPort?.IsOpen == true;
				},
				"EMERGENCY STOP!");
				return _StopCommand;
			}
		}

		public ICommand ClearCommand
		{
			get
			{
				_ClearCommand = _ClearCommand ?? new DelegateCommand(() =>
				{
					History.Clear();
				}, () =>
				{
					return _SerialPort?.IsOpen != true && History.Count > 0;
				},
				"Clear Data");
				return _ClearCommand;
			}
		}

		public ICommand ExportCommand
		{
			get
			{
				_ExportCommand = _ExportCommand ?? new DelegateCommand(async () =>
				{
					var data = await Task.Run(() =>
					{
						var sb = new StringBuilder("Throttle,Rpm,Voltage,Current,Thrust,KV");
						foreach (var item in History)
						{
							sb.AppendLine(item.ToCsvString());
						}
						return sb.ToString();
					});
					DataReady?.Invoke(this, data);
				}, () =>
				{
					return _SerialPort?.IsOpen != true && History.Count > 0;
				},
				"Export Data");
				return _ExportCommand;
			}
		}

		public ICommand RefreshCommand
		{
			get
			{
				_RefreshCommand = _RefreshCommand ?? new DelegateCommand(() =>
				{
					string[] ports = SerialPort.GetPortNames();
					if (ports != null)
					{
						var port = PortName;
						Ports = new List<string>(ports);
						if (!string.IsNullOrEmpty(port) && port.Contains(port))
						{
							PortName = port;
						}
						else
						{
							PortName = Ports.FirstOrDefault();
						}
					}

				}, () =>
				{
					return _SerialPort?.IsOpen != true;
				},
				"Refresh");
				return _RefreshCommand;
			}
		}

		public ICommand DisconnectCommand
		{
			get
			{
				_DisconnectCommand = _DisconnectCommand ?? new DelegateCommand(() =>
				{
					_SerialPort.Close();
					Connected = false;
				}, () =>
				{
					return _SerialPort?.IsOpen == true;
				},
				"Disconnect");
				return _DisconnectCommand;
			}
		}

		public ICommand ConnectCommand
		{
			get
			{
				_ConnectCommand = _ConnectCommand ?? new DelegateCommand(() =>
				{
					Thread readThread = new Thread(ReadCurrentPort);

					// Create a new SerialPort object with default settings.
					_SerialPort = new SerialPort
					{

						// Allow the user to set the appropriate properties.
						PortName = PortName,
						BaudRate = 38400,
						//_SerialPort.Parity = Parity.Even;
						//_SerialPort.DataBits = SetPortDataBits(_serialPort.DataBits);
						//_SerialPort.StopBits = SetPortStopBits(_serialPort.StopBits);
						//_SerialPort.Handshake = SetPortHandshake(_serialPort.Handshake);

						// Set the read/write timeouts
						//ReadTimeout = int.MaxValue,
						//WriteTimeout = 20,

						//WriteBufferSize = 50
                    };

                    _SerialPort.Open();
					Thread.Sleep(100);
					Connected = _SerialPort.IsOpen;

					Thread.Sleep(100);
					//send settings
					if (Connected && PolesCount != 0)
					{
						var buff = new byte[] { 101, (byte)PolesCount };
						try
						{
							_SerialPort.Write(buff, 0, 1);
							if (_SerialPort.ReadByte() == 101)
							{
								_SerialPort.Write(buff, 1, 1);
							}
						}
						catch (Exception)
						{
						}
					}

					readThread.Start();
				}, () =>
				{
					return !string.IsNullOrEmpty(PortName) && Ports.Contains(PortName) && ( _SerialPort == null || !_SerialPort.IsOpen);
				},
				"Connect");
				return _ConnectCommand;
			}
		}

		#region INotifyPropertyChanged members
		public void NotifyPropertyChanged([CallerMemberName] string propertyName = null)
		{
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
		}

		public event PropertyChangedEventHandler PropertyChanged;
		#endregion
	}	
}
