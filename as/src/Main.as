/**
 * ...
 * @author egocarib
 */

class Main 
{		
	public static function main(swfRoot:MovieClip):Void 
	{	
		var craftRoot:MovieClip = swfRoot._parent.Menu;
		var monitor:MenuBladeMonitor = new MenuBladeMonitor(craftRoot);
	}
	
	public function Main()
	{
	}
}