/* 作者：LESLIEXIN
 * 邮箱：LESLIEXIN@OUTLOOK.COM
 * 网站：HTTP://WWW.LESLIEXIN.COM/
 * 版权所有，使用时需保留此段作者信息。
 * 
 */
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace DragDropDemo
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            new Form2().ShowDialog();
        }

        private void button2_Click(object sender, EventArgs e)
        {
            new Form3().ShowDialog();
        }
    }
}
