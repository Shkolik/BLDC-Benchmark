using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Input;

namespace DataAnalizer
{
	/// <summary>
	/// This class facilitates associating a key binding in XAML markup to a command
	/// defined in a View Model by exposing a Command dependency property.
	/// The class derives from Freezable to work around a limitation in WPF when data-binding from XAML.
	/// </summary>
	public class CommandReference : Freezable, ICommand
	{
		/// <summary>
		/// CanExecuteChanged
		/// </summary>
		public event EventHandler CanExecuteChanged;

		/// <summary>
		/// CommandProperty
		/// </summary>
		public static readonly DependencyProperty CommandProperty = DependencyProperty.Register
		(
			"Command",
			typeof(ICommand),
			typeof(CommandReference),
			new PropertyMetadata(new PropertyChangedCallback((x, y) =>
			{
				var commandReference = x as CommandReference;

				if (y.OldValue is ICommand oldCommand)
					oldCommand.CanExecuteChanged -= commandReference.CanExecuteChanged;
				if (y.NewValue is ICommand newCommand)
					newCommand.CanExecuteChanged += commandReference.CanExecuteChanged;
			}))
		 );

		/// <summary>
		/// Command
		/// </summary>
		public ICommand Command
		{
			get { return (ICommand)GetValue(CommandProperty); }
			set { SetValue(CommandProperty, value); }
		}

		/// <summary>
		/// CanExecute
		/// </summary>
		/// <param name="parameter">parameter</param>
		/// <returns></returns>
		public bool CanExecute(object parameter)
		{
			if (Command != null) return Command.CanExecute(parameter);
			return false;
		}

		/// <summary>
		/// Execute
		/// </summary>
		/// <param name="parameter">parameter</param>
		public void Execute(object parameter)
		{
			Command.Execute(parameter);
		}

		/// <summary>
		/// CreateInstanceCore
		/// </summary>
		/// <returns></returns>
		protected override Freezable CreateInstanceCore()
		{
			throw new NotImplementedException();
		}
	}

	/// <summary>
	/// This class allows delegating the commanding logic to methods passed as parameters,
	/// and enables a View to bind commands to objects that are not part of the element tree.
	/// </summary>
	public class DelegateCommand : ICommand
	{

		private readonly Action _ExecuteMethod = null;
		private readonly Func<bool> _CanExecuteMethod = null;
		private bool _IsAutomaticRequeryDisabled = false;
		private List<WeakReference> _CanExecuteChangedHandlers;

		/// <summary>
		/// DelegateCommand
		/// </summary>
		/// <param name="executeMethod">executeMethod</param>
		public DelegateCommand(Action executeMethod)
			: this(executeMethod, null, null, false)
		{ }

		/// <summary>
		/// DelegateCommand
		/// </summary>
		/// <param name="executeMethod">executeMethod</param>
		/// <param name="canExecuteMethod">canExecuteMethod</param>
		public DelegateCommand(Action executeMethod, Func<bool> canExecuteMethod)
			: this(executeMethod, canExecuteMethod, null, false)
		{ }

		/// <summary>
		/// DelegateCommand
		/// </summary>
		/// <param name="executeMethod">executeMethod</param>
		/// <param name="canExecuteMethod">canExecuteMethod</param>
		/// <param name="text">Command description</param>
		public DelegateCommand(Action executeMethod, Func<bool> canExecuteMethod, string text)
			: this(executeMethod, canExecuteMethod, text, false)
		{ }

		/// <summary>
		/// DelegateCommand
		/// </summary>
		/// <param name="executeMethod">executeMethod</param>
		/// <param name="canExecuteMethod">canExecuteMethod</param>
		/// <param name="text">Command description</param>
		/// <param name="isAutomaticRequeryDisabled">isAutomaticRequeryDisabled</param>
		public DelegateCommand(Action executeMethod, Func<bool> canExecuteMethod, string text, bool isAutomaticRequeryDisabled)
		{
			_ExecuteMethod = executeMethod ?? throw new ArgumentNullException("executeMethod");
			_CanExecuteMethod = canExecuteMethod;
			_IsAutomaticRequeryDisabled = isAutomaticRequeryDisabled;
			Text = text;
		}

		/// <summary>
		/// Gets the text that describes this command.
		/// </summary>
		public string Text { get; }


		/// <summary>
		/// CanExecute
		/// </summary>
		/// <returns></returns>
		public bool CanExecute()
		{
			if (_CanExecuteMethod != null) return _CanExecuteMethod();
			return true;
		}

		/// <summary>
		///  Execution of the command
		/// </summary>
		public void Execute()
		{
			_ExecuteMethod?.Invoke();
		}

		/// <summary>
		///  Property to enable or disable CommandManager's automatic requery on this command
		/// </summary>
		public bool IsAutomaticRequeryDisabled
		{
			get { return _IsAutomaticRequeryDisabled; }
			set
			{
				if (_IsAutomaticRequeryDisabled != value)
				{
					if (value)
						CommandManagerHelper.RemoveHandlersFromRequerySuggested(_CanExecuteChangedHandlers);
					else
						CommandManagerHelper.AddHandlersToRequerySuggested(_CanExecuteChangedHandlers);
					_IsAutomaticRequeryDisabled = value;
				}
			}
		}

		/// <summary>
		/// RaiseCanExecuteChanged
		/// </summary>
		public void RaiseCanExecuteChanged()
		{
			OnCanExecuteChanged();
		}

		/// <summary>
		/// OnCanExecuteChanged
		/// </summary>
		protected virtual void OnCanExecuteChanged()
		{
			CommandManagerHelper.CallWeakReferenceHandlers(_CanExecuteChangedHandlers);
		}

		/// <summary>
		/// CanExecuteChanged
		/// </summary>
		public event EventHandler CanExecuteChanged
		{
			add
			{
				if (!_IsAutomaticRequeryDisabled)
					CommandManager.RequerySuggested += value;
				CommandManagerHelper.AddWeakReferenceHandler(ref _CanExecuteChangedHandlers, value, 2);
			}
			remove
			{
				if (!_IsAutomaticRequeryDisabled)
					CommandManager.RequerySuggested -= value;
				CommandManagerHelper.RemoveWeakReferenceHandler(_CanExecuteChangedHandlers, value);
			}
		}

		bool ICommand.CanExecute(object parameter)
		{
			return CanExecute();
		}

		void ICommand.Execute(object parameter)
		{
			Execute();
		}
	}

	/// <summary>
	/// This class allows delegating the commanding logic to methods passed as parameters,
	/// and enables a View to bind commands to objects that are not part of the element tree.
	/// </summary>
	/// <typeparam name="T">Type of the parameter passed to the delegates</typeparam>
	public class DelegateCommand<T> : ICommand
	{

		private readonly Action<T> _ExecuteMethod = null;
		private readonly Func<T, bool> _CanExecuteMethod = null;
		private bool _IsAutomaticRequeryDisabled = false;
		private List<WeakReference> _CanExecuteChangedHandlers;

		/// <summary>
		/// DelegateCommand
		/// </summary>
		/// <param name="executeMethod">executeMethod</param>
		public DelegateCommand(Action<T> executeMethod)
			: this(executeMethod, null, null, false) { }

		/// <summary>
		/// DelegateCommand
		/// </summary>
		/// <param name="executeMethod">executeMethod</param>
		/// <param name="canExecuteMethod">canExecuteMethod</param>
		public DelegateCommand(Action<T> executeMethod, Func<T, bool> canExecuteMethod)
			: this(executeMethod, canExecuteMethod, null, false)
		{ }

		/// <summary>
		/// DelegateCommand
		/// </summary>
		/// <param name="executeMethod">executeMethod</param>
		/// <param name="canExecuteMethod">canExecuteMethod</param>
		/// <param name="text">Command description</param>
		public DelegateCommand(Action<T> executeMethod, Func<T, bool> canExecuteMethod, string text)
			: this(executeMethod, canExecuteMethod, text, false)
		{ }

		/// <summary>
		/// DelegateCommand
		/// </summary>
		/// <param name="executeMethod">executeMethod</param>
		/// <param name="canExecuteMethod">canExecuteMethod</param>
		/// <param name="text">Command description</param>
		/// <param name="isAutomaticRequeryDisabled">isAutomaticRequeryDisabled</param>
		public DelegateCommand(Action<T> executeMethod, Func<T, bool> canExecuteMethod, string text, bool isAutomaticRequeryDisabled)
		{
			_ExecuteMethod = executeMethod ?? throw new ArgumentNullException("executeMethod");

			_CanExecuteMethod = canExecuteMethod;
			_IsAutomaticRequeryDisabled = isAutomaticRequeryDisabled;
			Text = text;
		}

		/// <summary>
		/// CanExecute
		/// </summary>
		/// <param name="parameter">parameter</param>
		/// <returns></returns>
		public bool CanExecute(T parameter)
		{
			if (_CanExecuteMethod != null) return _CanExecuteMethod(parameter);
			return true;
		}

		/// <summary>
		/// Execute
		/// </summary>
		/// <param name="parameter">parameter</param>
		public void Execute(T parameter)
		{
			_ExecuteMethod?.Invoke(parameter);
		}

		/// <summary>
		/// RaiseCanExecuteChanged
		/// </summary>
		public void RaiseCanExecuteChanged()
		{
			OnCanExecuteChanged();
		}

		/// <summary>
		/// OnCanExecuteChanged
		/// </summary>
		protected virtual void OnCanExecuteChanged()
		{
			CommandManagerHelper.CallWeakReferenceHandlers(_CanExecuteChangedHandlers);
		}

		/// <summary>
		/// Gets the text that describes this command.
		/// </summary>
		public string Text { get; }

		/// <summary>
		/// IsAutomaticRequeryDisabled
		/// </summary>
		public bool IsAutomaticRequeryDisabled
		{
			get { return _IsAutomaticRequeryDisabled; }
			set
			{
				if (_IsAutomaticRequeryDisabled != value)
				{
					if (value)
						CommandManagerHelper.RemoveHandlersFromRequerySuggested(_CanExecuteChangedHandlers);
					else
						CommandManagerHelper.AddHandlersToRequerySuggested(_CanExecuteChangedHandlers);

					_IsAutomaticRequeryDisabled = value;
				}
			}
		}

		/// <summary>
		/// CanExecuteChanged
		/// </summary>
		public event EventHandler CanExecuteChanged
		{
			add
			{
				if (!_IsAutomaticRequeryDisabled) CommandManager.RequerySuggested += value;
				CommandManagerHelper.AddWeakReferenceHandler(ref _CanExecuteChangedHandlers, value, 2);
			}
			remove
			{
				if (!_IsAutomaticRequeryDisabled) CommandManager.RequerySuggested -= value;
				CommandManagerHelper.RemoveWeakReferenceHandler(_CanExecuteChangedHandlers, value);
			}
		}

		bool ICommand.CanExecute(object parameter)
		{
			// if parameter have not expected type or
			// if T is of value type and the parameter is not
			// set yet, then return false if CanExecute delegate
			// exists, else return true
			if (parameter == null && typeof(T).IsValueType || !(parameter is T))
				return (_CanExecuteMethod == null);
			return CanExecute((T)parameter);
		}

		void ICommand.Execute(object parameter)
		{
			Execute((T)parameter);
		}
	}

	/// <summary>
	///     This class contains methods for the CommandManager that help avoid memory leaks by
	///     using weak references.
	/// </summary>
	internal class CommandManagerHelper
	{
		internal static Action<List<WeakReference>> CallWeakReferenceHandlers = x =>
		{
			if (x != null)
			{
				// Take a snapshot of the handlers before we call out to them since the handlers
				// could cause the array to me modified while we are reading it.

				var callers = new EventHandler[x.Count];
				int count = 0;

				for (int i = x.Count - 1; i >= 0; i--)
				{
					var reference = x[i];
					if (reference.Target is EventHandler handler)
					{
						callers[count] = handler;
						count++;
					}
					else
					{
						// Clean up old handlers that have been collected
						x.RemoveAt(i);
					}
				}

				// Call the handlers that we snapshotted
				for (int i = 0; i < count; i++)
				{
					EventHandler handler = callers[i];
					handler(null, EventArgs.Empty);
				}
			}
		};

		internal static Action<List<WeakReference>> AddHandlersToRequerySuggested = x =>
		{
			if (x != null)
			{
				x.ForEach(y =>
				{
					if (y.Target is EventHandler handler)
						CommandManager.RequerySuggested += handler;
				});
			}
		};

		internal static Action<List<WeakReference>> RemoveHandlersFromRequerySuggested = x =>
		{
			if (x != null)
			{
				x.ForEach(y =>
				{
					if (y.Target is EventHandler handler)
						CommandManager.RequerySuggested -= handler;
				});
			}
		};

		internal static void AddWeakReferenceHandler(ref List<WeakReference> handlers, EventHandler handler)
		{
			AddWeakReferenceHandler(ref handlers, handler, -1);
		}

		internal static void AddWeakReferenceHandler(ref List<WeakReference> handlers, EventHandler handler, int defaultListSize)
		{
			if (handlers == null)
			{
				handlers = defaultListSize > 0 ? new List<WeakReference>(defaultListSize) : new List<WeakReference>();
			}

			handlers.Add(new WeakReference(handler));
		}

		internal static Action<List<WeakReference>, EventHandler> RemoveWeakReferenceHandler = (x, y) =>
		{
			if (x != null)
			{
				for (int i = x.Count - 1; i >= 0; i--)
				{
					var reference = x[i];
					if (!(reference.Target is EventHandler existingHandler) || existingHandler == y)
					{
						// Clean up old handlers that have been collected
						// in addition to the handler that is to be removed.
						x.RemoveAt(i);
					}
				}
			}
		};
	}
}

