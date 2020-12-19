using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.DataVisualization.Charting;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

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
            DataContext = ViewModel;

            
        }

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
    }
}
