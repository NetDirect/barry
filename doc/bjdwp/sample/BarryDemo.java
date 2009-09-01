import net.rim.device.api.ui.UiApplication;
import net.rim.device.api.ui.container.MainScreen;
import net.rim.device.api.ui.Field;
import net.rim.device.api.ui.component.Dialog;
import net.rim.device.api.ui.component.LabelField;
import net.rim.device.api.ui.component.RichTextField;


class BarryDemo extends UiApplication
{
    public static void main(String[] args)
    {
        BarryDemo theApp = new BarryDemo();
        
        theApp.enterEventDispatcher();
    }

    private BarryDemo()
    {
        pushScreen(new BarryScreen());
    }    
}


final class BarryScreen extends MainScreen
{
    BarryScreen()
    {
        LabelField title = new LabelField("BarryDemo" , LabelField.ELLIPSIS | LabelField.USE_ALL_WIDTH);
        setTitle(title);

        System.out.println("Start test...");
        
        for (int i=0; i<10; i++) {
            String str = "Test number ";
            
            str = str + i;
            
            System.out.println(str);
        }

        System.out.println("End test...");
    }


    public void close()
    {
        System.exit(0);
        
        super.close();
    }   
}
