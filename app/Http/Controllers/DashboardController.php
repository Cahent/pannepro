<?php
namespace App\Http\Controllers;
use App\Models\Provider;
use App\Models\Region;
use Illuminate\Http\Request;
class DashboardController extends Controller {
    private function snapshot(?string $slug): array {
        $regions=Region::query()->orderBy('id')->get();
        $query=Provider::query()->with('regions')->orderBy('name');
        if($slug && $slug!=='all') $query->whereHas('regions',fn($q)=>$q->where('slug',$slug));
        $providers=$query->get()->map(fn($p)=>[
            'id'=>$p->id,'name'=>$p->name,'city'=>$p->city,'phone'=>$p->phone,
            'status'=>$p->liveStatus(),'last_seen'=>$p->last_seen_at?->format('d.m.Y H:i:s'),
            'regions'=>$p->regions->pluck('slug')->all(),
        ]);
        return ['regions'=>$regions->map(fn($r)=>['name'=>$r->name,'slug'=>$r->slug]),'providers'=>$providers,'counts'=>[
            'total'=>$providers->count(),'available'=>$providers->where('status','available')->count(),
            'busy'=>$providers->where('status','busy')->count(),
            'unavailable'=>$providers->where('status','unavailable')->count(),
            'disconnected'=>$providers->where('status','disconnected')->count(),
        ]];
    }
    public function index(Request $request){
        $region=$request->query('region','all'); if(!in_array($region,['all','nord','sued','west','ost'],true)) $region='all';
        return view('dashboard', ['snapshot'=>$this->snapshot($region),'region'=>$region]);
    }
    public function data(Request $request){
        $region=$request->query('region','all'); if(!in_array($region,['all','nord','sued','west','ost'],true)) abort(422);
        return response()->json($this->snapshot($region))->header('Cache-Control','no-store');
    }
}
