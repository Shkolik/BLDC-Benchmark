using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO.Ports;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace DataAnalizer
{
    public class DataPacket
    {
        public DataPacket()
        { 
        }

        public DataPacket(string input)
        {
            if (input != null)
            {
                var parts = input.Trim(new[] { ';', '\r', '\n', '\t' }).Split(new[] { ';' });
                foreach (var part in parts)
                {
                    var prop = part.Substring(0, 1);
                    var value = part.Substring(1);

                    switch(prop)
                    {
                        case "A":
                            if (decimal.TryParse(value, out decimal cur))
                            {
                                Current = cur;
                            }
                            break;
                        case "V":
                            if (decimal.TryParse(value, out decimal volt))
                            {
                                Voltage = volt;
                            }
                            break;
                        case "T":
                            if (decimal.TryParse(value, out decimal thr))
                            {
                                Thrust = thr;
                            }
                            break;
                        case "K":
                            if (int.TryParse(value, out int kv))
                            {
                                KV = kv;
                            }
                            break;
                        case "R":
                            if (int.TryParse(value, out int rpm))
                            {
                                RPM = rpm;
                            }
                            break;
                        case "G":
                            if (int.TryParse(value, out int gaz))
                            {
                                Throttle = gaz;
                            }
                            break;
                    }
                }
            }
        }

        public int Throttle { get; set; }

        public int RPM { get; set; }
        public int KV { get; set; }
        public decimal Current { get; set; }
        public decimal Thrust { get; set; }
        public decimal Voltage { get; set; }

        public bool NotEmpty()
        {
            return Throttle > 0 && (RPM > 0 || Thrust > 0 || KV > 0 || Current > 0);
        }

        public override string ToString()
        {
            return $"Throttle: {Throttle}\t RPM: {RPM}\t THRUST: {Thrust} Gramms\t Current: {Current} Amps\t Voltage: {Voltage} Volts\t KV: {KV}";
        }

        public string ToCsvString()
        {
            return $"{Throttle},{RPM},{Voltage},{Current},{Thrust},{KV}";
        }
    }
}
