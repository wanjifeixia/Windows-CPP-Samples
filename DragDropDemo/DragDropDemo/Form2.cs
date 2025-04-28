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
    public partial class Form2 : Form
    {
        public Form2()
        {
            InitializeComponent();
        }

        private Rectangle dragBox;

        private void label1_MouseDown(object sender, MouseEventArgs e)
        {
            dragBox = new Rectangle(new Point(e.X - (SystemInformation.DragSize.Width / 2),
                e.Y - (SystemInformation.DragSize.Height / 2)), SystemInformation.DragSize);
        }

        private void label1_MouseMove(object sender, MouseEventArgs e)
        {
            if ((e.Button & MouseButtons.Left) == MouseButtons.Left)
            {
                if (dragBox != Rectangle.Empty && !dragBox.Contains(e.X, e.Y))
                {
                    var effect = label1.DoDragDrop(label1.Text, DragDropEffects.All | DragDropEffects.Link);
                    if (effect == DragDropEffects.Move)
                        label1.Text = "";
                }
            }
        }

        private void label1_GiveFeedback(object sender, GiveFeedbackEventArgs e)
        {
            //
        }

        private void label3_DragEnter(object sender, DragEventArgs e)
        {
            if (!e.Data.GetDataPresent(typeof(string)))
            {
                e.Effect = DragDropEffects.None;
            }
        }

        private void label3_DragOver(object sender, DragEventArgs e)
        {
            if ((e.AllowedEffect & DragDropEffects.None) == DragDropEffects.None
                && (e.KeyState & (8 + 32)) == (8 + 32))
            {
                //CTRL+ALT
                e.Effect = DragDropEffects.None;
                label2.Text = $"按键状态：CTRL+ALT\r\n效果：None";
            }
            else if ((e.AllowedEffect & DragDropEffects.Link) == DragDropEffects.Link
                && (e.KeyState & (32)) == (32))
            {
                //ALT
                e.Effect = DragDropEffects.Link;
                label2.Text = $"按键状态：ALT\r\n效果：Link";
            }
            else if ((e.AllowedEffect & DragDropEffects.Copy) == DragDropEffects.Copy
                && (e.KeyState & (8)) == (8))
            {
                //CTRL
                e.Effect = DragDropEffects.Copy;
                label2.Text = $"按键状态：CTRL+ALT\r\n效果：Copy";
            }
            else if ((e.AllowedEffect & DragDropEffects.Move) == DragDropEffects.Move
                && (e.KeyState & (4)) == (4))
            {
                //SHIFT
                e.Effect = DragDropEffects.Move;
                label2.Text = $"按键状态：SHIFT\r\n效果：Move";
            }
            else if ((e.AllowedEffect & DragDropEffects.Copy) == DragDropEffects.Copy)
            {
                //无
                e.Effect = DragDropEffects.Copy;
                label2.Text = $"按键状态：无\r\n效果：Copy";
            }
            else
            {
                e.Effect = DragDropEffects.None;
                label2.Text = $"按键状态：无\r\n效果：None";
            }
        }

        private void label3_DragDrop(object sender, DragEventArgs e)
        {
            label3.Text = $"{e.Effect}：{(string)e.Data.GetData(typeof(string))}";
        }

        private void label1_MouseUp(object sender, MouseEventArgs e)
        {
            dragBox = Rectangle.Empty;
        }

        private void label3_Click(object sender, EventArgs e)
        {
            label3.Text = "";
        }
    }
}
