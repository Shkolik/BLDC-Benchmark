using Microsoft.Win32;
using RealTimeGraphX;
using RealTimeGraphX.DataPoints;
using RealTimeGraphX.WPF;
using System;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace DataAnalizer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        /// <summary>
        /// MainWindow
        /// </summary>
        public MainWindow()
        {
            InitializeComponent();

            ViewModel = new MainViewModel();
            ViewModel.DataReady += ViewModel_DataReady;
            ViewModel.History.CollectionChanged += History_CollectionChanged;
            DataContext = this;

            ThrottleController = new WpfGraphController<Int32DataPoint, Int32DataPoint>();
            ThrottleController.Range.MinimumY = 0;
            ThrottleController.Range.MaximumY = 100;
            ThrottleController.Range.MaximumX = 100;
            ThrottleController.Range.AutoY = true;
            ThrottleController.Range.AutoYFallbackMode = GraphRangeAutoYFallBackMode.MinMax;

            ThrottleController.DataSeriesCollection.Add(new WpfGraphDataSeries()
            {
                Name = "Series",
                Stroke = Colors.DodgerBlue,
            });

            ThrottleController.PushData(new Int32DataPoint(0), new Int32DataPoint(0));

            RpmController = new WpfGraphController<Int32DataPoint, Int32DataPoint>();
            RpmController.Range.MinimumY = 0;
            RpmController.Range.MaximumY = 35000;
            RpmController.Range.MaximumX = 100;
            RpmController.Range.AutoY = true;
            RpmController.Range.AutoYFallbackMode = GraphRangeAutoYFallBackMode.MinMax;

            RpmController.DataSeriesCollection.Add(new WpfGraphDataSeries()
            {
                Name = "Series",
                Stroke = Colors.IndianRed,
            });

            RpmController.PushData(new Int32DataPoint(0), new Int32DataPoint(0));
        }

        private void History_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            if (e.Action == System.Collections.Specialized.NotifyCollectionChangedAction.Add)
            {
                var idx = e.NewStartingIndex;

                foreach (var item in e.NewItems.OfType<DataPacket>())
                {
                    ThrottleController.PushData(new Int32DataPoint(idx), new Int32DataPoint(item.Throttle));
                    RpmController.PushData(new Int32DataPoint(idx), new Int32DataPoint(item.RPM));
                    idx++;
                }
            }
            else if (e.Action == System.Collections.Specialized.NotifyCollectionChangedAction.Reset)
            {
                ThrottleController.ClearCommand.Execute(null);
                RpmController.ClearCommand.Execute(null);

                ThrottleController.PushData(new Int32DataPoint(0), new Int32DataPoint(0));
                RpmController.PushData(new Int32DataPoint(0), new Int32DataPoint(0));
            }
        }

        public WpfGraphController<Int32DataPoint, Int32DataPoint> ThrottleController { get; set; }
        public WpfGraphController<Int32DataPoint, Int32DataPoint> RpmController { get; set; }

        private void ViewModel_DataReady(object sender, string e)
        {
            if (!string.IsNullOrEmpty(e))
            {
                var currentDir = Environment.CurrentDirectory;

                var dialog = new SaveFileDialog
                {
                    Filter = $"Comma separated files|*.csv"
                };
                if (dialog.ShowDialog() == true && !string.IsNullOrEmpty(dialog.FileName))
                {
                    using (var fs = new FileStream(dialog.FileName, FileMode.Create, FileAccess.Write))
                    using (TextWriter tw = new StreamWriter(fs))
                    {
                        tw.Write(e);
                    }
                }

                Environment.CurrentDirectory = currentDir;
            }
        }

        #region Properties.

        /// <summary>
        /// MainViewModel
        /// </summary>
        public MainViewModel ViewModel { get; set; }

        
        #endregion

        /// <summary>
        /// Scroll listbox to the bottom
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ListBox_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            if (sender is ListBox listBox && e.ExtentHeightChange > 0)
            {
                // Get the ScrollViewer object from the ListBox control
                Border border = (Border)VisualTreeHelper.GetChild(listBox, 0);
                ScrollViewer scrollViewer = (ScrollViewer)VisualTreeHelper.GetChild(border, 0);

                scrollViewer.ScrollToBottom();
            }
        }

        private void Window_KeyDown(object sender, KeyEventArgs e)
        {
            if(e.Key == Key.Space)
            {
                ViewModel.StopCommand.Execute(null);
            }
        }
    }
}
