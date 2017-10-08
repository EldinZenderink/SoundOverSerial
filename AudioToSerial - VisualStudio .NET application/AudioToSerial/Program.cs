using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;
using NAudio.CoreAudioApi;
using NAudio.Wave;
using System.Threading;
using System.Collections.Concurrent;

namespace AudioToSerial
{
    class Program
    {
        //public vars
        public static SerialPort ComPort = null;
        public static WasapiLoopbackCapture sourceStream;
        public static DirectSoundOut waveOut = null;
        public static bool mode = true;
        public static ConcurrentBag<UInt16[]> buffer;

        static void Main(string[] args)
        {

            //running setup program
            Console.WriteLine("Welcome to the STM32F0 Discovery DAC/PWM Audio setup program!");
            Console.WriteLine("Please connect your audio device to either pin PA4 for DAC mode, or PC7 for PWM!");
            Console.WriteLine("");
            Console.WriteLine("DAC plays at 48khz/12bit.");
            Console.WriteLine("PWM plays at 48khz/10bit.");
            Console.WriteLine("");
            Console.WriteLine("Before we begin, make sure that the baudrate is set to 960000 on the STM32F0!");
            Console.WriteLine("Compile and upload the included STM32F0 code to your device for hassle free audio playback ;)!");
            Console.WriteLine("");
            Console.WriteLine("");

            buffer = new ConcurrentBag<UInt16[]>();

            //lists all the avialable serial ports
            string[] ports = SerialPort.GetPortNames();
            int b = 0;
            Console.WriteLine("Select STM32F0 Discovery port by typing the corrosponding number:");
            foreach (string port in ports)
            {
                Console.WriteLine(b + " - " + port);
                b++;
            }

            bool succesconversion = false;
            int choiceint = 0;
            while (!succesconversion)
            {
                string choice = Console.ReadLine();
                succesconversion = int.TryParse(choice, out choiceint);                
            }
            string port_chosen = "";
            if(choiceint < ports.Length)
            {
                port_chosen = ports[choiceint];
            }

            //start connection with chosen serial port
            //baudrate of 960000 due to needing 2 bits to send data  = 48khz
            ComPort = new SerialPort();
            ComPort.PortName = port_chosen;
            ComPort.BaudRate = 960000;
            ComPort.Open();

            //gets all available devices for playback
            Console.WriteLine("Select where your audio should be played from by typing the corrosponding number:");
            MMDeviceEnumerator enumerator = new MMDeviceEnumerator();
            var devices = enumerator.EnumerateAudioEndPoints(DataFlow.All, DeviceState.Active);
            for (int i = 0; i < devices.Count; i++)
            {
                Console.WriteLine( i + " : " + devices[i].ToString());
            }

            succesconversion = false;
            choiceint = 0;

            while (!succesconversion)
            {             
                string choice = Console.ReadLine();
                succesconversion = int.TryParse(choice, out choiceint);
            }

            //start recording audio
            if (choiceint < devices.Count)
            {
                sourceStream = new WasapiLoopbackCapture(devices[choiceint]);
                sourceStream.DataAvailable += sourceStream_DataAvailable;
                sourceStream.StartRecording();
            }

            //play audio on device
            Thread play = new Thread(new ThreadStart(playbufferback));
            play.Start();

            //select which mode you want on the fly (need to manually change it on the stm32f0
            Console.WriteLine("10 BIT PWM MODE DISABLED");
            Console.WriteLine("Now using 12 bit DAC mode on pin PA4!");
            Console.WriteLine("If no sound or weird distorted sound is being produced, press the USER button on your DISCOVERY board!");
            Console.WriteLine("Sometimes your DISOVERY board needs a reset!");
            Console.WriteLine("You can switch to PWM by typing: pwm/PWM");
            Console.WriteLine("");
            Console.WriteLine("");

            while (true)
            {
                string modeselect = Console.ReadLine();
                if(modeselect.ToLower().Contains("dac"))
                {
                    Console.WriteLine("10 BIT PWM MODE DISABLED");
                    Console.WriteLine("Now using 12 bit DAC mode on pin PA4!");
                    Console.WriteLine("If the green LED on your Discovery is turned off, you are still in PWM mode, switch to DAC mode by pressing the USER button!");
                    Console.WriteLine("If no sound or weird distorted sound is being produced, press the USER button multiple times on your DISCOVERY board!");
                    Console.WriteLine("Sometimes your DISOVERY board needs a reset!");
                    Console.WriteLine("You can switch to PWM by typing: pwm/PWM");
                    Console.WriteLine("");
                    Console.WriteLine("");
                    mode = true;
                } else if(modeselect.ToLower().Contains("pwm"))
                {
                    Console.WriteLine("12 BIT DAC MODE DISABLED");
                    Console.WriteLine("Now using 10 bit PWM mode on PIN PC7!");
                    Console.WriteLine("If the green LED on your Discovery is turned on, you are still in DAC mode, switch to PWM mode by pressing the USER button!");
                    Console.WriteLine("If no sound or weird distorted sound is being produced, press the USER button multiple times on your DISCOVERY board!");
                    Console.WriteLine("Sometimes your DISOVERY board needs a reset!");
                    Console.WriteLine("You can switch to DAC by typing: dac/DAC");
                    Console.WriteLine("");
                    Console.WriteLine("");
                    mode = false;
                }
            }

        }

        //datastream events, fires when x amount of samples are retreived (around 4times/second)
        private static void sourceStream_DataAvailable(object sender, WaveInEventArgs e)
        {
            //calculates the amount of samples
            Int32 sample_count = e.BytesRecorded / (sourceStream.WaveFormat.BitsPerSample / 8);

            //converts stereo to mono
            //converts float to integer
            //depending on mode, either ~4096 resolution or 1024 resolution
            UInt16[] data = new UInt16[sample_count];
            
            for (int i = 0; i < sample_count; i++)
            {
                if (mode)// dac
                {
                    data[i] = (UInt16)((Single)(BitConverter.ToSingle(e.Buffer, i * 4) * 1000 - 32768 - 32258) * 4);
                } else if(!mode)   // pwm
                { 
                    data[i] = (UInt16)((Single)(BitConverter.ToSingle(e.Buffer, i * 4) * 1000 - 32768 - 32258));                    
                }
            }


            //get audio every second index ( Nyquist–Shannon theorem) - so 48khz = 24khz
            UInt16[] toplay = new UInt16[data.Length / 2];
            int c = 0;
            for (int sample = 0; sample < data.Length; sample += 2)
            {   
                toplay[c] = data[sample];
                c++;
            }

            //add samples to buffer
            buffer.Add(toplay);
        }

        private static void playbufferback()
        {
            //reads buffer and plays the values
            while (true)
            {
                while (buffer.Count > 0)
                {
                    UInt16[] value;
                    bool succes = buffer.TryTake(out value);
                    if (succes) {
                        int b = 0;
                        byte[] data = new byte[value.Length * 2];

                        //convert 16 bit samples to two 8 bit samples
                        for (int i = 0; i < value.Length; i++)
                        {                                                                
                            byte firstpart = (byte)(value[i] & 0xFF);
                            byte secondpart = (byte)(value[i] >> 8);
                            data[b] = firstpart;
                            data[b + 1] = secondpart;
                            b += 2;
                        }
                        //send over samples
                        ComPort.Write(data, 0, data.Length);
                    }   
                }
            }
        }
    }
}


