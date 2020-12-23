using System.ComponentModel;

namespace DataAnalizer
{
    public enum BaudRate
    {
        [Description("9600")]
        Slow = 9600,
        [Description("19200")]
        Normal = 19200,
        [Description("38400")]
        Fast = 38400
    }
}
