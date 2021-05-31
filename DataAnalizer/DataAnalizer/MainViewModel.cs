using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
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
		/// <summary>
		/// Flag for emulation process
		/// Ony for debuging
		/// </summary>
		private bool _Emulation;

		private ICommand _EmulateCommand;
		private ICommand _ConnectCommand;
		private ICommand _ExportCommand;
		private ICommand _ClearCommand;
		private ICommand _StopCommand;

		public event EventHandler<string> DataReady;

		private CancellationTokenSource _TokenSource;
		private SerialPort _SerialPort;

		public MainViewModel()
		{
			StartPortsDiscovery();
		}

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
				if (!string.IsNullOrEmpty(value) && value != _PortName)
				{
					LogMessage($"Port {value} selected");
				}
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
				string message = _Emulation ? "Emulation stopped" : $"{PortName} disconnected";
				if (value)
				{
					message = _Emulation ? "Emulation started" : $"{PortName} connected";					
				}

				LogMessage(message);
				NotifyPropertyChanged();
			}
		}

		private MotorType _MotorType = (MotorType)14;
		public MotorType MotorType
		{
			get
			{
				return _MotorType;
			}
			set
			{
				if (_MotorType != value)
				{
					_MotorType = value;					
					NotifyPropertyChanged();
				}
			}
		}

		private BaudRate _BaudRate = BaudRate.Fast;
		public BaudRate BaudRate
		{
			get
			{
				return _BaudRate;
			}
			set
			{
				if (_BaudRate != value)
				{
					_BaudRate = value;
					NotifyPropertyChanged();
				}
			}
		}

		private ObservableCollection<string> _Log = new ObservableCollection<string>();

		public ObservableCollection<string> Log
		{
			get
			{
				return _Log;
			}
			set
			{
				_Log = value;
				NotifyPropertyChanged();
			}
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
			while (_SerialPort?.IsOpen == true && !_TokenSource.IsCancellationRequested)
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
					var str = $"102,{Throttle};";
					_SerialPort.WriteLine(str);
				}
			}
			catch (TimeoutException)
			{ }
		}

        #region Commands

        public ICommand StopCommand
		{
			get
			{
				_StopCommand = _StopCommand ?? new DelegateCommand(() =>
				{
					Throttle = 0;
					LogMessage("EMERGENCY STOP executed!");
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
					LogMessage("Data was cleared");
				}, () =>
				{
					return !Connected && History.Count > 0;
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
						sb.AppendLine();
						foreach (var item in History)
						{
							sb.AppendLine(item.ToCsvString());
						}
						return sb.ToString();
					});
					DataReady?.Invoke(this, data);
				}, () =>
				{
					return !Connected && History.Count > 0;
				},
				"Export Data");
				return _ExportCommand;
			}
		}

		private const string NO_PORTS = "No ports found";
		private bool _DiscoveringPorts;
		private void StartPortsDiscovery()
		{
			if (!_DiscoveringPorts)
			{
				LogMessage("COM ports discovery started...");

				_DiscoveringPorts = true;
				Task.Run(() =>
				{
					while (true)
					{
						string[] ports = SerialPort.GetPortNames();
						if (ports?.Length > 0)
						{
							Application.Current.Dispatcher.BeginInvoke(DispatcherPriority.DataBind, new Action(() =>
							{
								var port = PortName;
								Ports = new List<string>(ports);
								if (!string.IsNullOrEmpty(port) && ports.Contains(port))
								{
									PortName = port;
								}
								else
								{
									PortName = Ports.FirstOrDefault();
								}
							}));
						}
						else
						{
							if (Connected && !_Emulation)
							{
								ConnectCommand.Execute(null);
							}

							Application.Current.Dispatcher.BeginInvoke(DispatcherPriority.DataBind, new Action(() =>
							{
								Ports = new List<string> { NO_PORTS };
								PortName = NO_PORTS;
							}));

						}

						Thread.Sleep(1000);
					}
				});
			}
		}

		public ICommand ConnectCommand
		{
			get
			{
				_ConnectCommand = _ConnectCommand ?? new DelegateCommand(() =>
				{
					if (Connected)
					{
						_TokenSource.Cancel();
						Thread.Sleep(100);
						_SerialPort?.Close();
						_SerialPort?.Dispose();
						Connected = false;
					}
					else
					{
						_SerialPort = new SerialPort
						{
							// Allow the user to set the appropriate properties.
							PortName = PortName,
							BaudRate = (int)BaudRate,
							DtrEnable = true
						};

						_SerialPort.Open();
						Connected = _SerialPort.IsOpen;

						//send settings
						if (Connected)
						{
							try
							{
								var result = HandShake();
								if (result)
								{
									_SerialPort.WriteLine($"101,{(int)MotorType},1;");
									LogMessage($"SetSettings command sent");

									result = HandShake();									
								}
								
								if(result)
                                {
									LogMessage($"SetSettings command acepted");
								}
								else
                                {
									LogMessage($"Settings not set - no responce from controller");
								}
							}
							catch (Exception ex)
							{
								LogMessage($"SetSettings command failed. {ex.Message}");
								return;
							}
						}

						UpdateThrottlePosition();

						_TokenSource = new CancellationTokenSource();

						LogMessage($"Starting data listener");
						Task.Run(() =>
						{
							ReadCurrentPort();
						}, _TokenSource.Token);
					}
				}, () =>
				{
					if(Connected)
                    {
						return _SerialPort?.IsOpen == true || _Emulation;
                    }
					return !string.IsNullOrEmpty(PortName) && PortName != NO_PORTS && Ports.Contains(PortName) && _SerialPort?.IsOpen != true;
				},
				"Connect");
				return _ConnectCommand;
			}
		}

		private bool HandShake()
		{
			var watch = new Stopwatch();
			watch.Start();

			while (watch.ElapsedMilliseconds < 4000 && _SerialPort.BytesToRead == 0)
			{
				Thread.Sleep(100);
			}

			return _SerialPort.BytesToRead == 1 && _SerialPort.ReadByte() == 101;
		}
		public ICommand EmulateCommand
		{
			get
			{
				_EmulateCommand = _EmulateCommand ?? new DelegateCommand(() =>
				{
					_Emulation = true;
					Connected = true;
					_TokenSource = new CancellationTokenSource();
					Task.Run(() => 
					{
						while (!_TokenSource.IsCancellationRequested)
						{
							var packet = new DataPacket { Throttle = Throttle, RPM = Throttle * 100, 
								Current = (decimal)(Throttle/3.0), Thrust = (decimal)(Throttle * 9.56), Voltage = 12.5m };

							Application.Current.Dispatcher.BeginInvoke(DispatcherPriority.DataBind, new Action(() =>
							{
								History.Add(packet);
							}));

							Thread.Sleep(200);
						}

						_Emulation = false;
					}, _TokenSource.Token);
				}, () =>
				{
#if DEBUG
					return true;
#else
					return false;
#endif
				},
				"Emulate");
				return _EmulateCommand;
			}
		}

		#endregion

		private void LogMessage(string message)
		{
			Application.Current.Dispatcher.BeginInvoke(DispatcherPriority.DataBind, new Action(() =>
			{
				Log.Add(message);
			}));
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
