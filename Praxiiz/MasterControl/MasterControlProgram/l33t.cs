using System;
using System.Collections.Generic;
using System.Text;

namespace MasterControlProgram
{
    static class l33t
    {
        static Random r = new Random(DateTime.Now.Millisecond);

        static bool CoinFlip()
        {
            return (r.Next(0x1000) + 1) > 0x800;
        }

        public static string w00t(string input)  //so serious
        {
            StringBuilder s = new StringBuilder();
            Random r = new Random(DateTime.Now.Millisecond);

            foreach (char c in input)
            {
                switch (Char.ToLower(c))
                {
                    case 'a':
                        s.Append('4');
                        break;
                    case 'b':
                        s.Append('8');
                        break;
                    case 'e':
                        s.Append('3');
                        break;
                    case 'g':
                        s.Append('9');
                        break;
                    case 'i':
                        s.Append('1');
                        break;
                    case 'o':
                        s.Append('0');
                        break;
                    case 's':
                        s.Append('5');
                        break;
                    case 't':
                        s.Append('7');
                        break;
                    case 'z':
                        if (CoinFlip()) s.Append('2');
                        else s.Append('Z');
                        break;
                    default:
                        if (CoinFlip()) s.Append(Char.ToLower(c));
                        else s.Append(Char.ToUpper(c));
                        break;
                }
            }
            return s.ToString();
        }

    }
}